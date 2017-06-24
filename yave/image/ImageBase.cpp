/*******************************
Copyright () 2016-2017 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/

#include "ImageBase.h"

#include <yave/buffer/StagingBufferMapping.h>
#include <yave/commands/CmdBufferRecorder.h>
#include <yave/device/Device.h>

namespace yave {

static u32 get_memory_type(const vk::PhysicalDeviceMemoryProperties& properties, u32 type_filter, vk::MemoryPropertyFlags flags) {
	for(u32 i = 0; i != properties.memoryTypeCount; i++) {
		auto memory_type = properties.memoryTypes[i];
		if(type_filter & (1 << i) && (memory_type.propertyFlags & flags) == flags) {
			return i;
		}
	}
	return fatal("Unable to alloc device memory.");
}

static vk::MemoryRequirements get_memory_reqs(DevicePtr dptr, vk::Image image) {
	return dptr->vk_device().getImageMemoryRequirements(image);
}

static void bind_image_memory(DevicePtr dptr, vk::Image image, vk::DeviceMemory memory) {
	dptr->vk_device().bindImageMemory(image, memory, 0);
}

static vk::DeviceMemory alloc_memory(DevicePtr dptr, vk::MemoryRequirements reqs) {
	return dptr->vk_device().allocateMemory(vk::MemoryAllocateInfo()
			.setAllocationSize(reqs.size)
			.setMemoryTypeIndex(get_memory_type(dptr->physical_device().vk_memory_properties(), reqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal))
		);
}

static vk::Image create_image(DevicePtr dptr, const math::Vec2ui& size, usize layers, usize mips, ImageFormat format, ImageUsage usage, ImageType type) {
	return dptr->vk_device().createImage(vk::ImageCreateInfo()
			.setSharingMode(vk::SharingMode::eExclusive)
			.setFlags(type == ImageType::Cube ? vk::ImageCreateFlagBits::eCubeCompatible : vk::ImageCreateFlags())
			.setArrayLayers(layers)
			.setExtent(vk::Extent3D(size.x(), size.y(), 1))
			.setFormat(format.vk_format())
			.setImageType(vk::ImageType::e2D)
			.setTiling(vk::ImageTiling::eOptimal)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setMipLevels(mips)
			.setUsage(vk::ImageUsageFlagBits(usage))
			.setSamples(vk::SampleCountFlagBits::e1)
		);
}

static auto get_copy_regions(const ImageData& data) {
	auto regions = core::vector_with_capacity<vk::BufferImageCopy>(data.mipmaps() * data.layers());

	for(usize l = 0; l != data.layers(); ++l) {
		for(usize m = 0; m != data.mipmaps(); ++m) {
			auto size = data.size(m);
			regions << vk::BufferImageCopy()
				.setBufferOffset(data.data_offset(l, m))
				.setImageExtent(vk::Extent3D(size.x(), size.y(), 1))
				.setImageSubresource(vk::ImageSubresourceLayers()
						.setAspectMask(data.format().vk_aspect())
						.setMipLevel(m)
						.setBaseArrayLayer(l)
						.setLayerCount(1)
					);
		}
	}

	return regions;
}

static auto get_staging_buffer(DevicePtr dptr, usize byte_size, const void* data) {
	auto staging_buffer = StagingBufferMapping::StagingBuffer(dptr, byte_size);
	auto mapping = CpuVisibleMapping(staging_buffer);
	memcpy(mapping.data(), data, byte_size);
	return staging_buffer;
}

static void upload_data(ImageBase& image, const ImageData& data) {
	DevicePtr dptr = image.device();

	auto staging_buffer = get_staging_buffer(dptr, data.combined_byte_size(), data.data());
	auto regions = get_copy_regions(data);

	CmdBufferRecorder recorder(dptr->create_disposable_cmd_buffer());

	recorder.transition_image(image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

	recorder.vk_cmd_buffer().copyBufferToImage(staging_buffer.vk_buffer(), image.vk_image(), vk::ImageLayout::eTransferDstOptimal, regions.size(), regions.data());

	recorder.transition_image(image, vk::ImageLayout::eTransferDstOptimal, vk_image_layout(image.usage()));

	RecordedCmdBuffer(std::move(recorder)).submit<SyncSubmit>(dptr->vk_queue(QueueFamily::Graphics));
}

static vk::ImageView create_view(DevicePtr dptr, vk::Image image, ImageFormat format, usize layers, usize mips, ImageType type) {
	return dptr->vk_device().createImageView(vk::ImageViewCreateInfo()
			.setImage(image)
			.setViewType(vk::ImageViewType(type))
			.setFormat(format.vk_format())
			.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(format.vk_aspect())
					.setBaseArrayLayer(0)
					.setBaseMipLevel(0)
					.setLayerCount(layers)
					.setLevelCount(mips)
				)
		);
}

static std::tuple<vk::Image, vk::DeviceMemory, vk::ImageView> alloc_image(DevicePtr dptr, const math::Vec2ui& size, usize layers, usize mips, ImageFormat format, ImageUsage usage, ImageType type) {
	auto image = create_image(dptr, size, layers, mips, format, usage, type);
	auto memory = alloc_memory(dptr, get_memory_reqs(dptr, image));
	bind_image_memory(dptr, image, memory);

	return std::tuple<vk::Image, vk::DeviceMemory, vk::ImageView>(image, memory, create_view(dptr, image, format, layers, mips, type));
}

static void check_layer_count(ImageType type, usize layers) {
	if(type == ImageType::TwoD && layers > 1) {
		fatal("Invalid layer count.");
	}
	if(type == ImageType::Cube && layers != 6) {
		fatal("Invalid layer count.");
	}
}




ImageBase::ImageBase(DevicePtr dptr, ImageUsage usage, ImageType type, const math::Vec2ui& size, const ImageData& data) :
		DeviceLinked(dptr),
		_size(size),
		_layers(data.layers()),
		_mips(data.mipmaps()),
		_format(data.format()),
		_usage(usage) {

	check_layer_count(type, _layers);

	auto tpl = alloc_image(dptr, size, _layers, _mips, _format, usage | vk::ImageUsageFlagBits::eTransferDst, type);
	std::tie(_image, _memory, _view) = tpl;

	upload_data(*this, data);
}

ImageBase::ImageBase(DevicePtr dptr, ImageFormat format, ImageUsage usage, const math::Vec2ui& size) :
		DeviceLinked(dptr),
		_size(size),
		_format(format),
		_usage(usage) {

	auto tpl = alloc_image(dptr, size, _layers, _mips, _format, usage, ImageType::TwoD);
	std::tie(_image, _memory, _view) = tpl;

	if(!is_attachment_usage(usage) && !is_storage_usage(usage)) {
		fatal("Texture images must be initialized.");
	}
}

ImageBase::~ImageBase() {
	destroy(_view);
	destroy(_memory);
	destroy(_image);
}

const math::Vec2ui& ImageBase::size() const {
	return _size;
}

usize ImageBase::mipmaps() const {
	return _mips;
}

usize ImageBase::layers() const {
	return _layers;
}

ImageFormat ImageBase::format() const {
	return _format;
}

ImageUsage ImageBase::usage() const {
	return _usage;
}

vk::ImageView ImageBase::vk_view() const {
	return _view;
}

vk::Image ImageBase::vk_image() const {
	return _image;
}

vk::DeviceMemory ImageBase::vk_device_memory() const {
	return _memory;
}

void ImageBase::swap(ImageBase& other) {
	DeviceLinked::swap(other);
	std::swap(_size, other._size);
	std::swap(_layers, other._layers);
	std::swap(_mips, other._mips);
	std::swap(_format, other._format);
	std::swap(_usage, other._usage);
	std::swap(_image, other._image);
	std::swap(_memory, other._memory);
	std::swap(_view, other._view);
}

}
