/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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
#include <yave/graphics/shaders/ShaderProgram.h>
#include <yave/graphics/barriers/Barrier.h>
#include <yave/meshes/MeshDrawData.h>

#include <yave/graphics/device/extensions/DebugUtils.h>

#include <y/core/ScratchPad.h>

namespace yave {

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
    bind_material_template(material.material_template(), material.descriptor_set(), 1);
}

void RenderPassRecorder::bind_material_template(const MaterialTemplate* material_template, DescriptorSetBase descriptor_set, u32 ds_offset) {
    if(material_template != _cache.material) {
        const GraphicPipeline& pipeline = material_template->compile(*_cmd_buffer._render_pass);
        vkCmdBindPipeline(vk_cmd_buffer(), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.vk_pipeline());

        _cache.material = material_template;
        _cache.pipeline_layout = pipeline.vk_pipeline_layout();
    }

    if(_main_descriptor_set) {
        vkCmdBindDescriptorSets(
            vk_cmd_buffer(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _cache.pipeline_layout,
            0,
            1, &_main_descriptor_set,
            0, nullptr
        );
        _main_descriptor_set = {};
    }

    if(!descriptor_set.is_null()) {
        const VkDescriptorSet vk_set = descriptor_set.vk_descriptor_set();
        vkCmdBindDescriptorSets(
            vk_cmd_buffer(),
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            _cache.pipeline_layout,
            ds_offset,
            1, &vk_set,
            0, nullptr
        );
    }
}




void RenderPassRecorder::set_main_descriptor_set(DescriptorSetBase ds_set) {
    _main_descriptor_set = ds_set.vk_descriptor_set();
}

void RenderPassRecorder::draw(const MeshDrawData& draw_data, u32 instance_count, u32 instance_index) {
    const std::array<AttribSubBuffer, 3> attribs = {
        draw_data.attrib_streams.positions,
        draw_data.attrib_streams.normals_tangents,
        draw_data.attrib_streams.uvs
    };

    bind_buffers(draw_data.triangle_buffer, attribs);

    VkDrawIndexedIndirectCommand indirect = draw_data.indirect_data;
    indirect.instanceCount = instance_count;
    indirect.firstInstance = instance_index;

    draw(indirect);
}

void RenderPassRecorder::draw(const VkDrawIndexedIndirectCommand& indirect) {
    vkCmdDrawIndexed(vk_cmd_buffer(),
        indirect.indexCount,
        indirect.instanceCount,
        indirect.firstIndex,
        indirect.vertexOffset,
        indirect.firstInstance
    );
}

void RenderPassRecorder::draw(const VkDrawIndirectCommand& indirect) {
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

void RenderPassRecorder::bind_buffers(IndexSubBuffer indices, core::Span<AttribSubBuffer> attribs) {
    bind_index_buffer(indices);
    bind_attrib_buffers(attribs);
}

void RenderPassRecorder::bind_index_buffer(IndexSubBuffer indices) {
    if(indices == _cache.index_buffer) {
        return;
    }

    _cache.index_buffer = indices;
    vkCmdBindIndexBuffer(vk_cmd_buffer(), indices.vk_buffer(), indices.byte_offset(), VK_INDEX_TYPE_UINT32);
}

void RenderPassRecorder::bind_attrib_buffers(core::Span<AttribSubBuffer> attribs) {
    const u32 attrib_count = u32(attribs.size());

    auto offsets = core::ScratchPad<VkDeviceSize>(attrib_count);
    auto buffers = core::ScratchPad<VkBuffer> (attrib_count);
    std::transform(attribs.begin(), attribs.end(), offsets.begin(), [](const auto& attr) { return attr.byte_offset(); });
    std::transform(attribs.begin(), attribs.end(), buffers.begin(), [](const auto& attr) { return attr.vk_buffer(); });

    vkCmdBindVertexBuffers(vk_cmd_buffer(), 0, attrib_count, buffers.data(), offsets.data());
}

void RenderPassRecorder::bind_per_instance_attrib_buffers(core::Span<AttribSubBuffer> per_instance) {
    const u32 attrib_count = u32(per_instance.size());

    auto offsets = core::ScratchPad<VkDeviceSize>(attrib_count);
    auto buffers = core::ScratchPad<VkBuffer>(attrib_count);

    std::transform(per_instance.begin(), per_instance.end(), offsets.begin(), [](const auto& buffer) { return buffer.byte_offset(); });
    std::transform(per_instance.begin(), per_instance.end(), buffers.begin(), [](const auto& buffer) { return buffer.vk_buffer(); });

    vkCmdBindVertexBuffers(vk_cmd_buffer(), ShaderProgram::per_instance_binding, attrib_count, buffers.data(), offsets.data());
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
    y_debug_assert(offset.x() >= 0.0f);
    y_debug_assert(offset.y() >= 0.0f);

    const VkRect2D scissor = {{offset.x(), offset.y()}, {size.x(), size.y()}};
    vkCmdSetScissor(vk_cmd_buffer(), 0, 1, &scissor);
}


// -------------------------------------------------- CmdBufferRecorder --------------------------------------------------



CmdBufferRecorder::CmdBufferRecorder(CmdBufferData* data) : _data(data) {
    VkCommandBufferBeginInfo begin_info = vk_struct();
    {
        begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    }

    vk_check(vkBeginCommandBuffer(vk_cmd_buffer(), &begin_info));
}

CmdBufferRecorder::~CmdBufferRecorder() {
    check_no_renderpass();
    y_always_assert(!_data, "CmdBufferRecorder has not been submitted");
}

CmdBufferRecorder::CmdBufferRecorder(CmdBufferRecorder&& other) {
    swap(other);
}

CmdBufferRecorder& CmdBufferRecorder::operator=(CmdBufferRecorder&& other) {
    swap(other);
    return *this;
}

void CmdBufferRecorder::swap(CmdBufferRecorder& other) {
    std::swap(_data, other._data);
    std::swap(_render_pass, other._render_pass);
}

VkCommandBuffer CmdBufferRecorder::vk_cmd_buffer() const {
    y_debug_assert(_data);
    return _data->vk_cmd_buffer();
}

ResourceFence CmdBufferRecorder::resource_fence() const {
    y_debug_assert(_data);
    return _data->resource_fence();
}

bool CmdBufferRecorder::is_inside_renderpass() const {
    return _render_pass;
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

    auto clear_values = core::ScratchPad<VkClearValue>(framebuffer.attachment_count() + 1);
    for(usize i = 0; i != framebuffer.attachment_count(); ++i) {
        clear_values[i] = VkClearValue{};
    }

    {
        VkClearValue depth_clear_value = {};
        depth_clear_value.depthStencil = VkClearDepthStencilValue{0.0f, 0}; // reversed Z
        clear_values.last() = depth_clear_value;
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

void CmdBufferRecorder::dispatch(const ComputeProgram& program, const math::Vec3ui& size, core::Span<DescriptorSetBase> descriptor_sets) {
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

    vkCmdDispatch(vk_cmd_buffer(), size.x(), size.y(), size.z());
}

void CmdBufferRecorder::dispatch_size(const ComputeProgram& program, const math::Vec3ui& size, core::Span<DescriptorSetBase> descriptor_sets) {
    math::Vec3ui dispatch_size;
    const math::Vec3ui program_size = program.local_size();
    for(usize i = 0; i != 3; ++i) {
        dispatch_size[i] = size[i] / program_size[i] + !!(size[i] % program_size[i]);
    }
    dispatch(program, dispatch_size, descriptor_sets);
}

void CmdBufferRecorder::dispatch_size(const ComputeProgram& program, const math::Vec2ui& size, core::Span<DescriptorSetBase> descriptor_sets) {
    dispatch_size(program, math::Vec3ui(size, 1), descriptor_sets);
}

void CmdBufferRecorder::barriers(core::Span<BufferBarrier> buffers, core::Span<ImageBarrier> images) {
    check_no_renderpass();

    if(buffers.is_empty() && images.is_empty()) {
        return;
    }

    auto image_barriers = core::ScratchPad<VkImageMemoryBarrier>(images.size());
    std::transform(images.begin(), images.end(), image_barriers.begin(), [](const auto& b) { return b.vk_barrier(); });

    auto buffer_barriers = core::ScratchPad<VkBufferMemoryBarrier>(buffers.size());
    std::transform(buffers.begin(), buffers.end(), buffer_barriers.begin(), [](const auto& b) { return b.vk_barrier(); });

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
    y_always_assert(src.byte_size() == dst.byte_size(), "Buffer size do not match.");

    VkBufferCopy copy = {};
    {
        copy.size = src.byte_size();
        copy.srcOffset = src.byte_offset();
        copy.dstOffset = dst.byte_offset();
    }

    vkCmdCopyBuffer(vk_cmd_buffer(), src.vk_buffer(), dst.vk_buffer(), 1, &copy);
}

void CmdBufferRecorder::blit(const SrcCopyImage& src, const DstCopyImage& dst) {
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

}

