/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

This code is licensed under the MIT License (MIT).

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
#include <yave/Device.h>

namespace yave {

static u32 get_memory_type(const vk::PhysicalDeviceMemoryProperties& properties, u32 type_filter, vk::MemoryPropertyFlags flags) {
	for(u32 i = 0; i != properties.memoryTypeCount; i++) {
		auto memory_type = properties.memoryTypes[i];
		if(type_filter & (1 << i) && (memory_type.propertyFlags & flags) == flags) {
			return i;
		}
	}
	return fatal("Unable to alloc device memory");
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

static vk::Image create_image(DevicePtr dptr, const math::Vec2ui& size, ImageFormat format, ImageUsage usage) {
	return dptr->vk_device().createImage(vk::ImageCreateInfo()
			.setSharingMode(vk::SharingMode::eExclusive)
			.setArrayLayers(1)
			.setExtent(vk::Extent3D(size.x(), size.y(), 1))
			.setFormat(format.vk_format())
			.setImageType(vk::ImageType::e2D)
			.setTiling(vk::ImageTiling::eOptimal)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setMipLevels(1)
			.setUsage(vk::ImageUsageFlagBits(usage))
			.setSamples(vk::SampleCountFlagBits::e1)
		);
}

static vk::BufferImageCopy get_copy_region(const math::Vec2ui& size, ImageFormat format) {
	return vk::BufferImageCopy()
		.setImageExtent(vk::Extent3D(size.x(), size.y(), 1))
		.setImageSubresource(vk::ImageSubresourceLayers()
				.setAspectMask(format.vk_aspect())
				.setMipLevel(0)
				.setBaseArrayLayer(0)
				.setLayerCount(1)
			);
}

static auto get_staging_buffer(DevicePtr dptr, usize byte_size, const void* data) {
	auto staging_buffer = StagingBufferMapping::StagingBuffer(dptr, byte_size);
	auto mapping = CpuVisibleMapping(staging_buffer);
	memcpy(mapping.data(), data, byte_size);
	return staging_buffer;
}

template<typename CommandBuffer>
void transition_image_layout(
		CommandBuffer& cmd_buffer,
		vk::Image image,
		ImageFormat format,
		vk::ImageLayout old_layout,
		vk::AccessFlags src_access,
		vk::ImageLayout new_layout,
		vk::AccessFlags dst_access) {

	auto barrier = vk::ImageMemoryBarrier()
			.setOldLayout(old_layout)
			.setNewLayout(new_layout)
			.setSrcAccessMask(src_access)
			.setDstAccessMask(dst_access)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setImage(image)
			.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(format.vk_aspect())
					.setBaseArrayLayer(0)
					.setBaseMipLevel(0)
					.setLayerCount(1)
					.setLevelCount(1)
				)
		;

	cmd_buffer.vk_cmd_buffer().pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::DependencyFlagBits::eByRegion,
			nullptr, nullptr, barrier);
}


void upload_data(DevicePtr dptr, vk::Image image, const math::Vec2ui& size, ImageFormat format, ImageUsage usage, const void* data) {
	usize byte_size = size.x() *size.y() *format.bpp();

	auto staging_buffer = get_staging_buffer(dptr, byte_size, data);
	auto regions = get_copy_region(size, format);

	auto cmd_buffer = CmdBufferRecorder(dptr->create_disposable_command_buffer());

	transition_image_layout(cmd_buffer, image, format,
			vk::ImageLayout::eUndefined, vk::AccessFlags(),
			vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eTransferWrite);

	cmd_buffer.vk_cmd_buffer().copyBufferToImage(staging_buffer.vk_buffer(), image, vk::ImageLayout::eTransferDstOptimal, regions);

	transition_image_layout(cmd_buffer, image, format,
			vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eTransferWrite,
			vk_image_layout(usage), vk_access_flags(usage));

	auto graphic_queue = dptr->vk_queue(QueueFamily::Graphics);
	cmd_buffer.end().submit(graphic_queue);
	graphic_queue.waitIdle();
}

vk::ImageView create_view(DevicePtr dptr, vk::Image image, ImageFormat format) {
	return dptr->vk_device().createImageView(vk::ImageViewCreateInfo()
			.setImage(image)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(format.vk_format())
			.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(format.vk_aspect())
					.setBaseArrayLayer(0)
					.setBaseMipLevel(0)
					.setLayerCount(1)
					.setLevelCount(1)
				)
		);
}


std::tuple<vk::Image, vk::DeviceMemory, vk::ImageView> alloc_image(DevicePtr dptr, const math::Vec2ui& size, ImageFormat format, ImageUsage usage) {
	auto image = create_image(dptr, size, format, usage);
	auto memory = alloc_memory(dptr, get_memory_reqs(dptr, image));
	bind_image_memory(dptr, image, memory);

	return std::tuple<vk::Image, vk::DeviceMemory, vk::ImageView>(image, memory, create_view(dptr, image, format));
}





ImageBase::ImageBase(DevicePtr dptr, ImageFormat format, ImageUsage usage, const math::Vec2ui& size, const void* data) :
		DeviceLinked(dptr),
		_size(size),
		_format(format) {

	auto tpl = alloc_image(dptr, size, format, data ? usage | vk::ImageUsageFlagBits::eTransferDst : usage);
	_image = std::get<0>(tpl);
	_memory = std::get<1>(tpl);
	_view = std::get<2>(tpl);

	if(data) {
		upload_data(device(), vk_image(), _size, format, usage, data);
	} else {
		auto cmd_buffer = CmdBufferRecorder(dptr->create_disposable_command_buffer());
		transition_image_layout(cmd_buffer, vk_image(), format,
				vk::ImageLayout::eUndefined, vk::AccessFlags(),
				vk_image_layout(usage), vk_access_flags(usage));

		auto graphic_queue = dptr->vk_queue(QueueFamily::Graphics);
		cmd_buffer.end().submit(graphic_queue);
		graphic_queue.waitIdle();
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

ImageFormat ImageBase::format() const {
	return _format;
}

vk::ImageView ImageBase::vk_view() const {
	return _view;
}

usize ImageBase::byte_size() const {
	return _size.x() *_size.y() *_format.bpp();
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
	std::swap(_format, other._format);
	std::swap(_image, other._image);
	std::swap(_memory, other._memory);
	std::swap(_view, other._view);
}

}
