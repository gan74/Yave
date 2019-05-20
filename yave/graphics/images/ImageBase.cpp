/*******************************
Copyright () 2016-2018 Grégoire Angerand

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

#include <yave/graphics/buffers/buffers.h>
#include <yave/graphics/buffers/Mapping.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/device/Device.h>

namespace yave {

static void bind_image_memory(DevicePtr dptr, vk::Image image, const DeviceMemory& memory) {
	dptr->vk_device().bindImageMemory(image, memory.vk_memory(), memory.vk_offset());
}

static vk::Image create_image(DevicePtr dptr, const math::Vec3ui& size, usize layers, usize mips, ImageFormat format, ImageUsage usage, ImageType type) {
	return dptr->vk_device().createImage(vk::ImageCreateInfo()
			.setSharingMode(vk::SharingMode::eExclusive)
			.setFlags(type == ImageType::Cube ? vk::ImageCreateFlagBits::eCubeCompatible : vk::ImageCreateFlags())
			.setArrayLayers(layers)
			.setExtent(vk::Extent3D(size.x(), size.y(), size.z()))
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
				.setImageExtent(vk::Extent3D(size.x(), size.y(), size.z()))
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
	y_debug_assert(data);
	auto staging_buffer = StagingBuffer(dptr, byte_size);
	std::memcpy(Mapping(staging_buffer).data(), data, byte_size);
	return staging_buffer;
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

static std::tuple<vk::Image, DeviceMemory, vk::ImageView> alloc_image(DevicePtr dptr, const math::Vec3ui& size, usize layers, usize mips, ImageFormat format, ImageUsage usage, ImageType type) {
	auto image = create_image(dptr, size, layers, mips, format, usage, type);
	auto memory = dptr->allocator().alloc(image);
	bind_image_memory(dptr, image, memory);

	return {image, std::move(memory), create_view(dptr, image, format, layers, mips, type)};
}

static void upload_data(ImageBase& image, const ImageData& data) {
	y_profile();
	DevicePtr dptr = image.device();

	auto staging_buffer = get_staging_buffer(dptr, data.combined_byte_size(), data.data());
	auto regions = get_copy_regions(data);

	CmdBufferRecorder recorder(dptr->create_disposable_cmd_buffer());

	{
		auto region = recorder.region("Image upload");
		recorder.transition_image(image, vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);
		recorder.vk_cmd_buffer().copyBufferToImage(staging_buffer.vk_buffer(), image.vk_image(), vk::ImageLayout::eTransferDstOptimal, regions.size(), regions.data());
		recorder.transition_image(image, vk::ImageLayout::eTransferDstOptimal, vk_image_layout(image.usage()));
	}

	dptr->graphic_queue().submit<SyncSubmit>(RecordedCmdBuffer(std::move(recorder)));
}

static void transition_image(ImageBase& image) {
	y_profile();
	DevicePtr dptr = image.device();

	CmdBufferRecorder recorder(dptr->create_disposable_cmd_buffer());
	recorder.transition_image(image, vk::ImageLayout::eUndefined, vk_image_layout(image.usage()));
	dptr->graphic_queue().submit<SyncSubmit>(RecordedCmdBuffer(std::move(recorder)));
}

static void check_layer_count(ImageType type, const math::Vec3ui& size, usize layers) {
	if(type == ImageType::TwoD && layers > 1) {
		y_fatal("Invalid layer count.");
	}
	if(type == ImageType::Cube && layers != 6) {
		y_fatal("Invalid layer count.");
	}
	if(size.z() == 0) {
		y_fatal("Invalid size.");
	}
	if(size.z() != 1 && type != ImageType::ThreeD) {
		y_fatal("Invalid size.");
	}
}


ImageBase::ImageBase(DevicePtr dptr, ImageFormat format, ImageUsage usage, const math::Vec3ui& size, ImageType type, usize layers, usize mips) :
		_size(size),
		_layers(layers),
		_mips(mips),
		_format(format),
		_usage(usage) {

	check_layer_count(type, _size, _layers);

	std::tie(_image, _memory, _view) = alloc_image(dptr, _size, _layers, _mips, _format, _usage, type);

	transition_image(*this);
}

ImageBase::ImageBase(DevicePtr dptr, ImageUsage usage, ImageType type, const ImageData& data) :
		_size(data.size()),
		_layers(data.layers()),
		_mips(data.mipmaps()),
		_format(data.format()),
		_usage(usage | ImageUsage::TransferDstBit) {

	check_layer_count(type, _size, _layers);

	std::tie(_image, _memory, _view) = alloc_image(dptr, _size, _layers, _mips, _format, _usage, type);

	upload_data(*this, data);
}

ImageBase::~ImageBase() {
	if(device()) {
		device()->destroy(_view);
		device()->destroy(_image);
		device()->destroy(std::move(_memory));
	}
}

DevicePtr ImageBase::device() const {
	return _memory.device();
}

const math::Vec3ui& ImageBase::image_size() const {
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

const DeviceMemory& ImageBase::device_memory() const {
	return _memory;
}

}
