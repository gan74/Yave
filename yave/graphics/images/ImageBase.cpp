/*******************************
Copyright () 2016-2020 Grégoire Angerand

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
#include <yave/graphics/queues/Queue.h>
#include <yave/graphics/memory/DeviceMemoryAllocator.h>
#include <yave/device/DeviceUtils.h>

namespace yave {

static void bind_image_memory(DevicePtr dptr, VkImage image, const DeviceMemory& memory) {
	vk_check(vkBindImageMemory(vk_device(dptr), image, memory.vk_memory(), memory.vk_offset()));
}

static VkImage create_image(DevicePtr dptr, const math::Vec3ui& size, usize layers, usize mips, ImageFormat format, ImageUsage usage, ImageType type) {
	y_debug_assert(usage != ImageUsage::TransferDstBit);

	VkImageCreateInfo create_info = vk_struct();
	{
		create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
		create_info.flags = type == ImageType::Cube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;
		create_info.arrayLayers = layers;
		create_info.extent = {size.x(), size.y(), size.z()};
		create_info.format = format.vk_format();
		create_info.imageType = VK_IMAGE_TYPE_2D;
		create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		create_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		create_info.mipLevels = mips;
		create_info.usage = VkImageUsageFlags(usage);
		create_info.samples = VK_SAMPLE_COUNT_1_BIT;
	}

	VkImage image = {};
	vk_check(vkCreateImage(vk_device(dptr), &create_info, vk_allocation_callbacks(dptr), &image));
	return image;
}

static auto get_copy_regions(const ImageData& data) {
	auto regions = core::vector_with_capacity<VkBufferImageCopy>(data.mipmaps() * data.layers());

	for(usize l = 0; l != data.layers(); ++l) {
		for(usize m = 0; m != data.mipmaps(); ++m) {
			const auto size = data.size(m);
			VkBufferImageCopy copy = {};
			{
				copy.bufferOffset = data.data_offset(l, m);
				copy.imageExtent = {size.x(), size.y(), size.z()};
				copy.imageSubresource.aspectMask = data.format().vk_aspect();
				copy.imageSubresource.mipLevel = m;
				copy.imageSubresource.baseArrayLayer = l;
				copy.imageSubresource.layerCount = 1;
			}
			regions << copy;
		}
	}

	return regions;
}

static auto stage_data(DevicePtr dptr, usize byte_size, const void* data) {
	y_profile();
	y_debug_assert(data);
	auto staging_buffer = StagingBuffer(dptr, byte_size);
	std::memcpy(Mapping(staging_buffer).data(), data, byte_size);
	return staging_buffer;
}

static VkImageView create_view(DevicePtr dptr, VkImage image, ImageFormat format, usize layers, usize mips, ImageType type) {
	VkImageViewCreateInfo create_info = vk_struct();
	{
		create_info.image = image;
		create_info.viewType = VkImageViewType(type);
		create_info.format = format.vk_format();
		create_info.subresourceRange.aspectMask = format.vk_aspect();
		create_info.subresourceRange.layerCount = layers;
		create_info.subresourceRange.levelCount = mips;
	}

	VkImageView view = {};
	vk_check(vkCreateImageView(vk_device(dptr), &create_info, vk_allocation_callbacks(dptr), &view));
	return view;
}

static std::tuple<VkImage, DeviceMemory, VkImageView> alloc_image(DevicePtr dptr, const math::Vec3ui& size, usize layers, usize mips, ImageFormat format, ImageUsage usage, ImageType type) {
	y_profile();
	const auto image = create_image(dptr, size, layers, mips, format, usage, type);
	auto memory = device_allocator(dptr).alloc(image);
	bind_image_memory(dptr, image, memory);

	return {image, std::move(memory), create_view(dptr, image, format, layers, mips, type)};
}

static void upload_data(ImageBase& image, const ImageData& data) {
	y_profile();
	DevicePtr dptr = image.device();

	const auto staging_buffer = stage_data(dptr, data.combined_byte_size(), data.data());
	const auto regions = get_copy_regions(data);

	CmdBufferRecorder recorder(create_disposable_cmd_buffer(dptr));

	{
		const auto region = recorder.region("Image upload");
		recorder.barriers({ImageBarrier::transition_barrier(image, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)});
		vkCmdCopyBufferToImage(recorder.vk_cmd_buffer(), staging_buffer.vk_buffer(), image.vk_image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, regions.size(), regions.data());
		recorder.barriers({ImageBarrier::transition_barrier(image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, vk_image_layout(image.usage()))});
	}

	graphic_queue(dptr).submit<SyncSubmit>(RecordedCmdBuffer(std::move(recorder)));
}

static void transition_image(ImageBase& image) {
	y_profile();
	DevicePtr dptr = image.device();

	CmdBufferRecorder recorder(create_disposable_cmd_buffer(dptr));
	recorder.barriers({ImageBarrier::transition_barrier(image, VK_IMAGE_LAYOUT_UNDEFINED, vk_image_layout(image.usage()))});
	graphic_queue(dptr).submit<SyncSubmit>(RecordedCmdBuffer(std::move(recorder)));
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
	if(const DevicePtr dptr = device()) {
		device_destroy(dptr, _view);
		device_destroy(dptr, _image);
		device_destroy(dptr, std::move(_memory));
	}
}

DevicePtr ImageBase::device() const {
	return _memory.device();
}

bool ImageBase::is_null() const {
	return !device();
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

VkImageView ImageBase::vk_view() const {
	return _view;
}

VkImage ImageBase::vk_image() const {
	return _image;
}

const DeviceMemory& ImageBase::device_memory() const {
	return _memory;
}

}
