/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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

#include "Barrier.h"

namespace yave {

static vk::AccessFlags vk_layout_access_flags(vk::ImageLayout layout) {
	switch(layout) {
		case vk::ImageLayout::eUndefined:
			return vk::AccessFlags();

		case vk::ImageLayout::eColorAttachmentOptimal:
			return vk::AccessFlagBits::eColorAttachmentRead | vk::AccessFlagBits::eColorAttachmentWrite;

		case vk::ImageLayout::eDepthStencilAttachmentOptimal:
			return vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite;

		case vk::ImageLayout::eDepthStencilReadOnlyOptimal:
			return vk::AccessFlagBits::eDepthStencilAttachmentRead;

		case vk::ImageLayout::eShaderReadOnlyOptimal:
			return vk::AccessFlagBits::eShaderRead;

		case vk::ImageLayout::eTransferDstOptimal:
			return vk::AccessFlagBits::eTransferWrite;

		case vk::ImageLayout::eTransferSrcOptimal:
			return vk::AccessFlagBits::eTransferRead;

		case vk::ImageLayout::ePresentSrcKHR:
			return vk::AccessFlagBits::eMemoryRead;

		case vk::ImageLayout::eGeneral: // assume storage image
			return //vk::AccessFlagBits::eMemoryRead | vk::AccessFlagBits::eMemoryWrite |
				   vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;

		default:
			break;
	}

	return y_fatal("Unsupported layout transition.");
}

vk::ImageMemoryBarrier create_image_barrier(
		vk::Image image,
		ImageFormat format,
		usize layers,
		usize mips,
		vk::ImageLayout old_layout,
		vk::ImageLayout new_layout) {

	return vk::ImageMemoryBarrier()
			.setOldLayout(old_layout)
			.setNewLayout(new_layout)
			.setSrcAccessMask(vk_layout_access_flags(old_layout))
			.setDstAccessMask(vk_layout_access_flags(new_layout))
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setImage(image)
			.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(format.vk_aspect())
					.setLayerCount(layers)
					.setLevelCount(mips)
				)
		;
}


static vk::AccessFlags vk_dst_access_flags(PipelineStage dst) {
	if((dst & PipelineStage::AllShadersBit) != PipelineStage::None) {
		return vk::AccessFlagBits::eShaderRead;
	}
	switch(dst) {
		case PipelineStage::TransferBit:
			return vk::AccessFlagBits::eTransferRead;

		case PipelineStage::HostBit:
			return vk::AccessFlagBits::eHostRead;

		default:
			break;
	}
	return y_fatal("Unsuported pipeline stage.");
}

static vk::AccessFlags vk_src_access_flags(PipelineStage src) {
	if((src & PipelineStage::AllShadersBit) != PipelineStage::None) {
		return vk::AccessFlagBits::eShaderWrite;
	}
	switch(src) {
		case PipelineStage::TransferBit:
			return vk::AccessFlagBits::eTransferWrite;

		case PipelineStage::HostBit:
			return vk::AccessFlagBits::eHostWrite;

		case PipelineStage::ColorAttachmentOutBit:
			return vk::AccessFlagBits::eColorAttachmentWrite;

		default:
			break;
	}
	return y_fatal("Unsuported pipeline stage.");
}

static vk::ImageMemoryBarrier create_barrier(vk::Image image, ImageFormat format, usize layers, usize mips, ImageUsage usage, PipelineStage src, PipelineStage dst) {
	auto layout = vk_image_layout(usage);
	return vk::ImageMemoryBarrier()
			.setOldLayout(layout)
			.setNewLayout(layout)
			.setSrcAccessMask(vk_src_access_flags(src))
			.setDstAccessMask(vk_dst_access_flags(dst))
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setImage(image)
			.setSubresourceRange(vk::ImageSubresourceRange()
					.setAspectMask(format.vk_aspect())
					.setLayerCount(layers)
					.setLevelCount(mips)
				)
		;
}

static vk::BufferMemoryBarrier create_barrier(vk::Buffer buffer, usize size, usize offset, PipelineStage src, PipelineStage dst) {
	return vk::BufferMemoryBarrier()
			.setSrcAccessMask(vk_src_access_flags(src))
			.setDstAccessMask(vk_dst_access_flags(dst))
			.setBuffer(buffer)
			.setSize(size)
			.setOffset(offset)
			.setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
			.setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
		;
}


ImageBarrier::ImageBarrier(const ImageBase& image, PipelineStage src, PipelineStage dst) :
		_barrier(create_barrier(image.vk_image(), image.format(), image.layers(), image.mipmaps(), image.usage(), src, dst)),
		_src(src), _dst(dst) {
}


BufferBarrier::BufferBarrier(const BufferBase& buffer, PipelineStage src, PipelineStage dst) :
		_barrier(create_barrier(buffer.vk_buffer(), buffer.byte_size(), 0, src, dst)),
		_src(src), _dst(dst) {
}

BufferBarrier::BufferBarrier(const SubBufferBase& buffer, PipelineStage src, PipelineStage dst) :
		_barrier(create_barrier(buffer.vk_buffer(), buffer.byte_size(), buffer.byte_offset(), src, dst)),
		_src(src), _dst(dst) {
}




}
