/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
	return dptr->get_vk_device().getImageMemoryRequirements(image);
}

static void bind_image_memory(DevicePtr dptr, vk::Image image, vk::DeviceMemory memory) {
	dptr->get_vk_device().bindImageMemory(image, memory, 0);
}

static vk::DeviceMemory alloc_memory(DevicePtr dptr, vk::MemoryRequirements reqs) {
	return dptr->get_vk_device().allocateMemory(vk::MemoryAllocateInfo()
			.setAllocationSize(reqs.size)
			.setMemoryTypeIndex(get_memory_type(dptr->get_physical_device().get_vk_memory_properties(), reqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal))
		);
}

static vk::Image create_image(DevicePtr dptr, const math::Vec2ui& size, ImageFormat format, vk::ImageUsageFlags usage) {
	return dptr->get_vk_device().createImage(vk::ImageCreateInfo()
			.setSharingMode(vk::SharingMode::eExclusive)
			.setArrayLayers(1)
			.setExtent(vk::Extent3D(size.x(), size.y(), 1))
			.setFormat(format.get_vk_format())
			.setImageType(vk::ImageType::e2D)
			.setTiling(vk::ImageTiling::eOptimal)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setMipLevels(1)
			.setUsage(usage)
			.setSamples(vk::SampleCountFlagBits::e1)
		);
}

static vk::BufferImageCopy get_copy_region(const math::Vec2ui& size, ImageFormat format) {
	return vk::BufferImageCopy()
		.setImageExtent(vk::Extent3D(size.x(), size.y(), 1))
		.setImageSubresource(vk::ImageSubresourceLayers()
				.setAspectMask(format.get_vk_aspect())
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
					.setAspectMask(format.get_vk_aspect())
					.setBaseArrayLayer(0)
					.setBaseMipLevel(0)
					.setLayerCount(1)
					.setLevelCount(1)
				)
		;

	cmd_buffer.get_vk_cmd_buffer().pipelineBarrier(
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::PipelineStageFlagBits::eTopOfPipe,
			vk::DependencyFlagBits::eByRegion,
			nullptr, nullptr, barrier);
}


void upload_data(DevicePtr dptr, vk::Image image, const math::Vec2ui& size, ImageFormat format, ImageUsage usage, const void* data) {
	usize byte_size = size.x() *size.y() *format.get_bpp();

	auto staging_buffer = get_staging_buffer(dptr, byte_size, data);
	auto regions = get_copy_region(size, format);

	auto cmd_buffer = CmdBufferRecorder(dptr->create_disposable_command_buffer());

	transition_image_layout(cmd_buffer, image, format,
			vk::ImageLayout::eUndefined, vk::AccessFlags(),
			vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eTransferWrite);

	cmd_buffer.get_vk_cmd_buffer().copyBufferToImage(staging_buffer.get_vk_buffer(), image, vk::ImageLayout::eTransferDstOptimal, regions);

	transition_image_layout(cmd_buffer, image, format,
			vk::ImageLayout::eTransferDstOptimal, vk::AccessFlagBits::eTransferWrite,
			usage.get_vk_image_layout(), usage.get_vk_access_flags());

	auto graphic_queue = dptr->get_vk_queue(QueueFamily::Graphics);
	cmd_buffer.end().submit(graphic_queue);
	graphic_queue.waitIdle();
}

vk::ImageView create_view(DevicePtr dptr, vk::Image image, ImageFormat format) {
	return dptr->get_vk_device().createImageView(vk::ImageViewCreateInfo()
			.setImage(image)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(format.get_vk_format())
			.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(format.get_vk_aspect())
					.setBaseArrayLayer(0)
					.setBaseMipLevel(0)
					.setLayerCount(1)
					.setLevelCount(1)
				)
		);
}


std::tuple<vk::Image, vk::DeviceMemory, vk::ImageView> alloc_image(DevicePtr dptr, const math::Vec2ui& size, ImageFormat format, ImageUsage usage) {
	auto image = create_image(dptr, size, format, usage.get_vk_image_usage());
	auto memory = alloc_memory(dptr, get_memory_reqs(dptr, image));
	bind_image_memory(dptr, image, memory);

	return std::tuple<vk::Image, vk::DeviceMemory, vk::ImageView>(image, memory, create_view(dptr, image, format));
}





ImageBase::ImageBase(DevicePtr dptr, ImageFormat format, ImageUsage usage, const math::Vec2ui& size, const void* data) :
		DeviceLinked(dptr),
		_size(size),
		_format(format) {

	auto tpl = alloc_image(dptr, size, format, data ? ImageUsage(usage | vk::ImageUsageFlagBits::eTransferDst) : usage);
	_image = std::get<0>(tpl);
	_memory = std::get<1>(tpl);
	_view = std::get<2>(tpl);

	if(data) {
		upload_data(get_device(), get_vk_image(), _size, get_format(), usage, data);
	} else {
		auto cmd_buffer = CmdBufferRecorder(dptr->create_disposable_command_buffer());
		transition_image_layout(cmd_buffer, get_vk_image(), format,
				vk::ImageLayout::eUndefined, vk::AccessFlags(),
				usage.get_vk_image_layout(), usage.get_vk_access_flags());

		auto graphic_queue = dptr->get_vk_queue(QueueFamily::Graphics);
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

ImageFormat ImageBase::get_format() const {
	return _format;
}

vk::ImageView ImageBase::get_vk_view() const {
	return _view;
}

usize ImageBase::byte_size() const {
	return _size.x() *_size.y() *_format.get_bpp();
}

vk::Image ImageBase::get_vk_image() const {
	return _image;
}

vk::DeviceMemory ImageBase::get_vk_device_memory() const {
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
