/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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


static VkAccessFlags vk_layout_access_flags(VkImageLayout layout) {
    switch(layout) {
        case VK_IMAGE_LAYOUT_UNDEFINED:
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return 0;

        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_ACCESS_SHADER_READ_BIT;

        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_ACCESS_TRANSFER_READ_BIT;

        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_ACCESS_TRANSFER_WRITE_BIT;

        case VK_IMAGE_LAYOUT_GENERAL:
            // assume storage image
            return //VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT |
                   VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;

        default:
            break;
    }

    y_fatal("Unsupported layout transition");
}

static VkAccessFlags vk_dst_access_flags(PipelineStage dst) {
    if((dst & PipelineStage::AllShadersBit) != PipelineStage::None) {
        return VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_UNIFORM_READ_BIT;
    }
    switch(dst) {
        case PipelineStage::TransferBit:
            return VK_ACCESS_TRANSFER_READ_BIT;

        case PipelineStage::HostBit:
            return VK_ACCESS_HOST_READ_BIT;

        /*case PipelineStage::VertexInputBit:
            return VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT;*/

        case PipelineStage::BeginOfPipe:
        case PipelineStage::EndOfPipe:
            return VK_ACCESS_MEMORY_READ_BIT;

        default:
            break;
    }

    y_fatal("Unsuported pipeline stage");
}

static VkAccessFlags vk_src_access_flags(PipelineStage src) {
    if((src & PipelineStage::AllShadersBit) != PipelineStage::None) {
        return VK_ACCESS_SHADER_WRITE_BIT;
    }
    switch(src) {
        case PipelineStage::TransferBit:
            return VK_ACCESS_TRANSFER_WRITE_BIT;

        case PipelineStage::HostBit:
            return VK_ACCESS_HOST_WRITE_BIT;

        case PipelineStage::ColorAttachmentOutBit:
            return VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        case PipelineStage::BeginOfPipe:
        case PipelineStage::EndOfPipe:
            return VK_ACCESS_MEMORY_WRITE_BIT;

        default:
            break;
    }

    y_fatal("Unsuported pipeline stage");
}

static VkPipelineStageFlags vk_barrier_stage(VkAccessFlags access) {
    if(access == 0 || access == VK_ACCESS_MEMORY_READ_BIT) {
        return VK_PIPELINE_STAGE_HOST_BIT;
    }
    if(access & (VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT)) {
        return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
               VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    }
    if(access & (VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT)) {
        return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    }
    if(access & (VK_ACCESS_TRANSFER_READ_BIT | VK_ACCESS_TRANSFER_WRITE_BIT)) {
        return VK_PIPELINE_STAGE_TRANSFER_BIT;
    }
    if(access & (VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT)) {
        return VK_PIPELINE_STAGE_VERTEX_SHADER_BIT |
               VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT |
               VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
    }

    y_fatal("Unknown access flags");
}


static PipelineStage vk_src_barrier_stage(VkAccessFlags access) {
    return PipelineStage(vk_barrier_stage(access));
}

static PipelineStage vk_dst_barrier_stage(VkAccessFlags access) {
    return PipelineStage(vk_barrier_stage(access));
}




static VkImageMemoryBarrier create_barrier(VkImage image, ImageFormat format, usize layers, usize mips, VkImageLayout old_layout, VkImageLayout new_layout) {
    VkImageMemoryBarrier barrier = vk_struct();
    {
        barrier.oldLayout = old_layout;
        barrier.newLayout = new_layout;
        barrier.srcAccessMask = vk_layout_access_flags(old_layout);
        barrier.dstAccessMask = vk_layout_access_flags(new_layout);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;

        barrier.subresourceRange.aspectMask = format.vk_aspect();
        barrier.subresourceRange.layerCount = u32(layers);
        barrier.subresourceRange.levelCount = u32(mips);
    }
    return barrier;
}


static VkImageMemoryBarrier create_barrier(VkImage image, ImageFormat format, usize layers, usize mips, ImageUsage usage, PipelineStage src, PipelineStage dst) {
    VkImageMemoryBarrier barrier = vk_struct();
    {
        const VkImageLayout layout = vk_image_layout(usage);

        barrier.oldLayout = layout;
        barrier.newLayout = layout;
        barrier.srcAccessMask = vk_src_access_flags(src);
        barrier.dstAccessMask = vk_dst_access_flags(dst);
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;

        barrier.subresourceRange.aspectMask = format.vk_aspect();
        barrier.subresourceRange.layerCount = u32(layers);
        barrier.subresourceRange.levelCount = u32(mips);
    }
    return barrier;
}

static VkBufferMemoryBarrier create_barrier(VkBuffer buffer, usize size, usize offset, PipelineStage src, PipelineStage dst) {
    VkBufferMemoryBarrier barrier = vk_struct();
    {
        Y_TODO(uniform buffers needs to use VK_ACCESS_UNIFORM_READ_BIT)
        barrier.srcAccessMask = vk_src_access_flags(src);
        barrier.dstAccessMask = vk_dst_access_flags(dst);
        barrier.buffer = buffer;
        barrier.size = size;
        barrier.offset = offset;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    }
    return barrier;
}


ImageBarrier::ImageBarrier(const ImageBase& image, PipelineStage src, PipelineStage dst) :
        _barrier(create_barrier(image.vk_image(), image.format(), image.layers(), image.mipmaps(), image.usage(), src, dst)),
        _src(src), _dst(dst) {
}

ImageBarrier ImageBarrier::transition_barrier(const ImageBase& image, VkImageLayout src_layout, VkImageLayout dst_layout) {
    ImageBarrier barrier;
    barrier._barrier = create_barrier(image.vk_image(), image.format(), image.layers(), image.mipmaps(), src_layout, dst_layout);
    barrier._src = vk_src_barrier_stage(barrier._barrier.srcAccessMask);
    barrier._dst = vk_dst_barrier_stage(barrier._barrier.dstAccessMask);

    return barrier;
}

ImageBarrier ImageBarrier::transition_to_barrier(const ImageBase& image, VkImageLayout dst_layout) {
    return transition_barrier(image, vk_image_layout(image.usage()), dst_layout);
}

ImageBarrier ImageBarrier::transition_from_barrier(const ImageBase& image, VkImageLayout src_layout) {
    return transition_barrier(image, src_layout, vk_image_layout(image.usage()));
}

VkImageMemoryBarrier ImageBarrier::vk_barrier() const {
    return _barrier;
}

PipelineStage ImageBarrier::dst_stage() const {
    return _dst;
}

PipelineStage ImageBarrier::src_stage() const {
    return _src;
}



BufferBarrier::BufferBarrier(const BufferBase& buffer, PipelineStage src, PipelineStage dst) :
        _barrier(create_barrier(buffer.vk_buffer(), buffer.byte_size(), 0, src, dst)),
        _src(src), _dst(dst) {
}

BufferBarrier::BufferBarrier(const SubBufferBase& buffer, PipelineStage src, PipelineStage dst) :
        _barrier(create_barrier(buffer.vk_buffer(), buffer.byte_size(), buffer.byte_offset(), src, dst)),
        _src(src), _dst(dst) {
}

VkBufferMemoryBarrier BufferBarrier::vk_barrier() const {
    return _barrier;
}

PipelineStage BufferBarrier::dst_stage() const {
    return _dst;
}

PipelineStage BufferBarrier::src_stage() const {
    return _src;
}


}

