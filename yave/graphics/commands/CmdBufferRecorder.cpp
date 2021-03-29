/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include "CmdBufferRecorder.h"

#include <yave/material/Material.h>
#include <yave/material/MaterialTemplate.h>
#include <yave/graphics/descriptors/DescriptorSet.h>
#include <yave/graphics/framebuffer/Framebuffer.h>
#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/graphics/barriers/Barrier.h>
#include <yave/graphics/device/Queue.h>

#include <yave/graphics/device/extensions/DebugUtils.h>

namespace yave {

#define YAVE_VK_CMD


// -------------------------------------------------- CmdBufferRegion --------------------------------------------------

CmdBufferRegion::~CmdBufferRegion() {
    if(_buffer) {
        if(const DebugUtils* debug = debug_utils()) {
            debug->end_region(_buffer);
        }
    }
}

CmdBufferRegion::CmdBufferRegion(const CmdBufferRecorder& cmd_buffer, const char* name, const math::Vec4& color) : _buffer(cmd_buffer.vk_cmd_buffer()) {
    if(const DebugUtils* debug = debug_utils()) {
        debug->begin_region(_buffer, name, color);
    }
}


// -------------------------------------------------- RenderPassRecorder --------------------------------------------------

RenderPassRecorder::RenderPassRecorder(CmdBufferRecorder& cmd_buffer, const Viewport& viewport) : _cmd_buffer(cmd_buffer) {
    set_viewport(viewport);
    set_scissor(math::Vec2i(viewport.offset), math::Vec2ui(viewport.extent));
}

RenderPassRecorder::~RenderPassRecorder() {
    _cmd_buffer.end_renderpass();
}

void RenderPassRecorder::bind_material(const Material& material) {
    bind_material(material.material_template(), {material.descriptor_set()});
}

void RenderPassRecorder::bind_material(const MaterialTemplate* material, DescriptorSetList descriptor_sets) {
    bind_pipeline(material->compile(*_cmd_buffer._render_pass), descriptor_sets);
}

void RenderPassRecorder::bind_pipeline(const GraphicPipeline& pipeline, DescriptorSetList descriptor_sets) {
    YAVE_VK_CMD;

    vkCmdBindPipeline(vk_cmd_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.vk_pipeline());

    if(!descriptor_sets.is_empty()) {
        vkCmdBindDescriptorSets(
            vk_cmd_buffer(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            pipeline.vk_pipeline_layout(),
            0,
            u32(descriptor_sets.size()), reinterpret_cast<const VkDescriptorSet*>(descriptor_sets.data()),
            0, nullptr
        );
    }
}


void RenderPassRecorder::draw(const VkDrawIndexedIndirectCommand& indirect) {
    YAVE_VK_CMD;

    vkCmdDrawIndexed(vk_cmd_buffer(),
        indirect.indexCount,
        indirect.instanceCount,
        indirect.firstIndex,
        indirect.vertexOffset,
        indirect.firstInstance
    );
}

void RenderPassRecorder::draw(const VkDrawIndirectCommand& indirect) {
    YAVE_VK_CMD;

    vkCmdDraw(vk_cmd_buffer(),
        indirect.vertexCount,
        indirect.instanceCount,
        indirect.firstVertex,
        indirect.firstInstance
    );
}

void RenderPassRecorder::draw_indexed(usize index_count) {
    VkDrawIndexedIndirectCommand command = {};
    command.indexCount = u32(index_count);
    command.instanceCount = 1;
    draw(command);
}

void RenderPassRecorder::draw_array(usize vertex_count) {
    VkDrawIndirectCommand command = {};
    command.vertexCount = u32(vertex_count);
    command.instanceCount = 1;
    draw(command);
}

void RenderPassRecorder::bind_buffers(const SubBuffer<BufferUsage::IndexBit>& indices,
                                      const SubBuffer<BufferUsage::AttributeBit>& per_vertex,
                                      core::Span<SubBuffer<BufferUsage::AttributeBit>> per_instance) {
    YAVE_VK_CMD;

    bind_index_buffer(indices);
    bind_attrib_buffers(per_vertex, per_instance);
}

void RenderPassRecorder::bind_index_buffer(const SubBuffer<BufferUsage::IndexBit>& indices) {
    YAVE_VK_CMD;

    vkCmdBindIndexBuffer(vk_cmd_buffer(), indices.vk_buffer(), indices.byte_offset(), VK_INDEX_TYPE_UINT32);
}

void RenderPassRecorder::bind_attrib_buffers(const SubBuffer<BufferUsage::AttributeBit>& per_vertex, core::Span<SubBuffer<BufferUsage::AttributeBit>> per_instance) {
    YAVE_VK_CMD;

    if(per_instance.is_empty()) {
        const VkDeviceSize offset = per_vertex.byte_offset();
        const VkBuffer buffer = per_vertex.vk_buffer();
        vkCmdBindVertexBuffers(vk_cmd_buffer(), 0, 1, &buffer, &offset);
    } else {
        const bool has_per_vertex = !per_vertex.is_null();
        const u32 attrib_count = u32(per_instance.size()) + has_per_vertex;

        auto offsets = core::vector_with_capacity<VkDeviceSize>(attrib_count);
        auto buffers = core::vector_with_capacity<VkBuffer>(attrib_count);

        if(has_per_vertex) {
            offsets << per_vertex.byte_offset();
            buffers << per_vertex.vk_buffer();
        }

        std::transform(per_instance.begin(), per_instance.end(), std::back_inserter(offsets), [](const auto& buffer) { return buffer.byte_offset(); });
        std::transform(per_instance.begin(), per_instance.end(), std::back_inserter(buffers), [](const auto& buffer) { return buffer.vk_buffer(); });

        vkCmdBindVertexBuffers(vk_cmd_buffer(), u32(!has_per_vertex), attrib_count, buffers.data(), offsets.data());
    }

}

CmdBufferRegion RenderPassRecorder::region(const char* name, const math::Vec4& color) {
    return _cmd_buffer.region(name, color);
}

VkCommandBuffer RenderPassRecorder::vk_cmd_buffer() const {
    return _cmd_buffer.vk_cmd_buffer();
}

const Viewport& RenderPassRecorder::viewport() const {
    return _viewport;
}

void RenderPassRecorder::set_viewport(const Viewport& vp) {
    YAVE_VK_CMD;

    y_debug_assert(vp.offset.x() >= 0.0f);
    y_debug_assert(vp.offset.y() >= 0.0f);

    _viewport = vp;
    const VkViewport v {
        vp.offset.x(), vp.offset.y(),
        vp.extent.x(), vp.extent.y(),
        vp.depth.x(), vp.depth.y()
    };
    vkCmdSetViewport(vk_cmd_buffer(), 0, 1, &v);
}

void RenderPassRecorder::set_scissor(const math::Vec2i& offset, const math::Vec2ui& size) {
    YAVE_VK_CMD;

    y_debug_assert(offset.x() >= 0.0f);
    y_debug_assert(offset.y() >= 0.0f);

    const VkRect2D scissor = {{offset.x(), offset.y()}, {size.x(), size.y()}};
    vkCmdSetScissor(vk_cmd_buffer(), 0, 1, &scissor);
}


// -------------------------------------------------- CmdBufferRecorder --------------------------------------------------

CmdBufferRecorder::CmdBufferRecorder(CmdBuffer&& base) : CmdBuffer(std::move(base)) {

    VkCommandBufferBeginInfo begin_info = vk_struct();
    {
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }

    vk_check(vkBeginCommandBuffer(vk_cmd_buffer(), &begin_info));
}

CmdBufferRecorder::~CmdBufferRecorder() {
    check_no_renderpass();
}

void CmdBufferRecorder::end_renderpass() {
    y_always_assert(_render_pass, "CmdBufferRecorder has no render pass");

    vkCmdEndRenderPass(vk_cmd_buffer());
    _render_pass = nullptr;
}

void CmdBufferRecorder::check_no_renderpass() const {
    y_always_assert(!_render_pass, "Command can not be used or destoryed while it has a RenderPassRecorder.");
}

CmdBufferRegion CmdBufferRecorder::region(const char* name, const math::Vec4& color) {
    return CmdBufferRegion(*this, name, color);
}


RenderPassRecorder CmdBufferRecorder::bind_framebuffer(const Framebuffer& framebuffer) {
    check_no_renderpass();

    auto clear_values = core::vector_with_capacity<VkClearValue>(framebuffer.attachment_count() + 1);
    for(usize i = 0; i != framebuffer.attachment_count(); ++i) {
        clear_values << VkClearValue{};
    }

    {
        VkClearValue depth_clear_value = {};
        depth_clear_value.depthStencil = VkClearDepthStencilValue{0.0f, 0}; // reversed Z
        clear_values << depth_clear_value;
    }

    VkRenderPassBeginInfo begin_info = vk_struct();
    {
        begin_info.renderArea = {{0, 0}, {framebuffer.size().x(), framebuffer.size().y()}};
        begin_info.renderPass = framebuffer.render_pass().vk_render_pass();
        begin_info.framebuffer = framebuffer.vk_framebuffer();
        begin_info.pClearValues = clear_values.begin();
        begin_info.clearValueCount = u32(clear_values.size());
    }


    vkCmdBeginRenderPass(vk_cmd_buffer(), &begin_info, VK_SUBPASS_CONTENTS_INLINE);
    _render_pass = &framebuffer.render_pass();

    return RenderPassRecorder(*this, Viewport(framebuffer.size()));
}

void CmdBufferRecorder::dispatch(const ComputeProgram& program, const math::Vec3ui& size, DescriptorSetList descriptor_sets, const PushConstant& push_constants) {
    YAVE_VK_CMD;

    check_no_renderpass();

    vkCmdBindPipeline(vk_cmd_buffer(), VK_PIPELINE_BIND_POINT_COMPUTE, program.vk_pipeline());

    if(!descriptor_sets.is_empty()) {
        vkCmdBindDescriptorSets(vk_cmd_buffer(),
            VK_PIPELINE_BIND_POINT_COMPUTE,
            program.vk_pipeline_layout(),
            0,
            u32(descriptor_sets.size()), reinterpret_cast<const VkDescriptorSet*>(descriptor_sets.data()),
            0, nullptr);
    }

    if(!push_constants.is_empty()) {
        vkCmdPushConstants(vk_cmd_buffer(), program.vk_pipeline_layout(), VK_SHADER_STAGE_COMPUTE_BIT, 0, u32(push_constants.size()), push_constants.data());
    }

    vkCmdDispatch(vk_cmd_buffer(), size.x(), size.y(), size.z());
}

void CmdBufferRecorder::dispatch_size(const ComputeProgram& program, const math::Vec3ui& size, DescriptorSetList descriptor_sets, const PushConstant& push_constants) {
    math::Vec3ui dispatch_size;
    for(usize i = 0; i != 3; ++i) {
        dispatch_size[i] = size[i] / program.local_size()[i] + !!(size[i] % program.local_size()[i]);
    }
    dispatch(program, dispatch_size, descriptor_sets, push_constants);
}

void CmdBufferRecorder::dispatch_size(const ComputeProgram& program, const math::Vec2ui& size, DescriptorSetList descriptor_sets, const PushConstant& push_constants) {
    dispatch_size(program, math::Vec3ui(size, 1), descriptor_sets, push_constants);
}

void CmdBufferRecorder::barriers(core::Span<BufferBarrier> buffers, core::Span<ImageBarrier> images) {
    YAVE_VK_CMD;

    check_no_renderpass();

    if(buffers.is_empty() && images.is_empty()) {
        return;
    }

    auto image_barriers = core::vector_with_capacity<VkImageMemoryBarrier>(images.size());
    std::transform(images.begin(), images.end(), std::back_inserter(image_barriers), [](const auto& b) { return b.vk_barrier(); });

    auto buffer_barriers = core::vector_with_capacity<VkBufferMemoryBarrier>(buffers.size());
    std::transform(buffers.begin(), buffers.end(), std::back_inserter(buffer_barriers), [](const auto& b) { return b.vk_barrier(); });

    PipelineStage src_mask = PipelineStage::None;
    PipelineStage dst_mask = PipelineStage::None;

    for(const auto& b : buffers) {
        src_mask = src_mask | b.src_stage();
        dst_mask = dst_mask | b.dst_stage();
    }

    for(const auto& b : images) {
        src_mask = src_mask | b.src_stage();
        dst_mask = dst_mask | b.dst_stage();
    }

    vkCmdPipelineBarrier(
        vk_cmd_buffer(),
        VkPipelineStageFlags(src_mask),
        VkPipelineStageFlags(dst_mask),
        VK_DEPENDENCY_BY_REGION_BIT,
        0, nullptr,
        u32(buffer_barriers.size()), buffer_barriers.data(),
        u32(image_barriers.size()), image_barriers.data()
    );
}

void CmdBufferRecorder::barriers(core::Span<BufferBarrier> buffers) {
    barriers(buffers, {});
}

void CmdBufferRecorder::barriers(core::Span<ImageBarrier> images) {
    barriers({}, images);
}

void CmdBufferRecorder::full_barrier() {
    VkMemoryBarrier barrier = vk_struct();
    barrier.srcAccessMask =
        VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
        VK_ACCESS_INDEX_READ_BIT |
        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
        VK_ACCESS_UNIFORM_READ_BIT |
        VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
        VK_ACCESS_SHADER_READ_BIT |
        VK_ACCESS_SHADER_WRITE_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_TRANSFER_READ_BIT |
        VK_ACCESS_TRANSFER_WRITE_BIT |
        VK_ACCESS_HOST_READ_BIT |
        VK_ACCESS_HOST_WRITE_BIT
    ;
    barrier.dstAccessMask =
        VK_ACCESS_INDIRECT_COMMAND_READ_BIT |
        VK_ACCESS_INDEX_READ_BIT |
        VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT |
        VK_ACCESS_UNIFORM_READ_BIT |
        VK_ACCESS_INPUT_ATTACHMENT_READ_BIT |
        VK_ACCESS_SHADER_READ_BIT |
        VK_ACCESS_SHADER_WRITE_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_READ_BIT |
        VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT |
        VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT |
        VK_ACCESS_TRANSFER_READ_BIT |
        VK_ACCESS_TRANSFER_WRITE_BIT |
        VK_ACCESS_HOST_READ_BIT |
        VK_ACCESS_HOST_WRITE_BIT
    ;
    vkCmdPipelineBarrier(
        vk_cmd_buffer(),
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT,
        1, &barrier,
        0, nullptr,
        0, nullptr
    );
}

void CmdBufferRecorder::barriered_copy(const ImageBase& src,  const ImageBase& dst) {
    YAVE_VK_CMD;

    {
        const std::array image_barriers = {
                ImageBarrier::transition_to_barrier(src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
                ImageBarrier::transition_barrier(dst, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL),
            };
        barriers(image_barriers);
    }

    {
        y_always_assert(src.image_size() == dst.image_size(), "Image size do not match.");

        VkImageCopy copy = {};
        {
            copy.extent = {src.image_size().x(), src.image_size().y(), src.image_size().z()};
            copy.srcSubresource.aspectMask = src.format().vk_aspect();
            copy.srcSubresource.layerCount = u32(src.layers());
            copy.dstSubresource.aspectMask = dst.format().vk_aspect();
            copy.dstSubresource.layerCount = u32(dst.layers());
        }

        vkCmdCopyImage(vk_cmd_buffer(),
                       src.vk_image(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                       dst.vk_image(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                       1, &copy);
    }

    {
        const std::array image_barriers = {
                ImageBarrier::transition_from_barrier(src, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL),
                ImageBarrier::transition_from_barrier(dst, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
            };
        barriers(image_barriers);
    }
}

void CmdBufferRecorder::copy(const SrcCopyBuffer& src, const DstCopyBuffer& dst) {
    YAVE_VK_CMD;

    y_always_assert(src.byte_size() == dst.byte_size(), "Buffer size do not match.");

    VkBufferCopy copy = {};
    {
        copy.size = src.byte_size();
        copy.srcOffset = src.byte_offset();
        copy.dstOffset = dst.byte_offset();
    }

    vkCmdCopyBuffer(vk_cmd_buffer(), src.vk_buffer(), dst.vk_buffer(), 1, &copy);
}

/*void CmdBufferRecorder::copy(const SrcCopyImage& src, const DstCopyImage& dst) {
    YAVE_VK_CMD;

    y_always_assert(src.size() == dst.size(), "Image size do not match.");

    VkImageCopy copy = {};
    {
        copy.extent = {src.size().x(), src.size().y(), 1};
        copy.srcSubresource.aspectMask = src.format().vk_aspect();
        copy.srcSubresource.layerCount = 1;
        copy.dstSubresource.aspectMask = dst.format().vk_aspect();
        copy.dstSubresource.layerCount = 1;
    }

    vkCmdCopyImage(vk_cmd_buffer(),
                   src.vk_image(), vk_image_layout(src.usage()),
                   dst.vk_image(), vk_image_layout(dst.usage()),
                   1, &copy);
}*/

void CmdBufferRecorder::blit(const SrcCopyImage& src, const DstCopyImage& dst) {
    YAVE_VK_CMD;

    VkImageBlit blit = {};
    {
        blit.srcSubresource.aspectMask = src.format().vk_aspect();
        blit.srcSubresource.layerCount = 1;
        blit.dstSubresource.aspectMask = dst.format().vk_aspect();
        blit.dstSubresource.layerCount = 1;
    }

    vkCmdBlitImage(vk_cmd_buffer(), src.vk_image(), vk_image_layout(src.usage()), dst.vk_image(), vk_image_layout(dst.usage()), 1, &blit, VK_FILTER_LINEAR);
}

void CmdBufferRecorder::transition_image(ImageBase& image, VkImageLayout src, VkImageLayout dst) {
    barriers({ImageBarrier::transition_barrier(image, src, dst)});
}

void CmdBufferRecorder::submit(SyncPolicy policy) {
    submit(graphic_queue(), policy);
}

void CmdBufferRecorder::submit(const Queue& queue, SyncPolicy policy) {
    check_no_renderpass();
    vk_check(vkEndCommandBuffer(vk_cmd_buffer()));

    queue.submit(*this);

    switch(policy) {
        case SyncPolicy::Async:
            // nothing
        break;

        case SyncPolicy::Sync: {
            wait();
        } break;
    }
}

CmdBuffer CmdBufferRecorder::finish() && {
    Y_TODO(This is super sketchy, it needs to be removed ASAP)
    check_no_renderpass();
    vk_check(vkEndCommandBuffer(vk_cmd_buffer()));
    return std::move(*this); // uuuh ?
}

#undef YAVE_VK_CMD

}

