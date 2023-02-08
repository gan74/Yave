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
#ifndef YAVE_GRAPHICS_COMMANDS_CMDBUFFERRECORDER_H
#define YAVE_GRAPHICS_COMMANDS_CMDBUFFERRECORDER_H

#include "CmdBufferData.h"

#include <yave/graphics/framebuffer/Viewport.h>
#include <yave/graphics/images/ImageView.h>
#include <yave/graphics/descriptors/DescriptorSetBase.h>
#include <yave/graphics/buffers/buffers.h>

namespace yave {

class CmdBufferRegion : NonCopyable {
    public:
        CmdBufferRegion() = default;
        ~CmdBufferRegion();

        CmdBufferRegion(CmdBufferRegion&& other) {
            std::swap(_buffer, other._buffer);
        }

        CmdBufferRegion& operator=(CmdBufferRegion&& other) {
            std::swap(_buffer, other._buffer);
            return *this;
        }

    private:
        friend class CmdBufferRecorder;

        CmdBufferRegion(const CmdBufferRecorder& cmd_buffer, const char* name, const math::Vec4& color);

        VkCommandBuffer _buffer = {};
};

class RenderPassRecorder final : NonMovable {
    public:
        ~RenderPassRecorder();

        // specific
        void bind_material(const Material& material);
        void bind_material_template(const MaterialTemplate* material_template, DescriptorSetBase descriptor_set, u32 ds_offset = 0);

        void set_main_descriptor_set(DescriptorSetBase ds_set);

        void draw(const MeshDrawData& draw_data, u32 instance_count = 1, u32 instance_index = 0);

        void draw(const VkDrawIndexedIndirectCommand& indirect);
        void draw(const VkDrawIndirectCommand& indirect);

        void draw_indexed(usize index_count);
        void draw_array(usize vertex_count, usize instance_count = 1);

        void bind_mesh_buffers(const MeshBufferData& mesh_buffers);
        void bind_index_buffer(IndexSubBuffer indices);
        void bind_attrib_buffers(core::Span<AttribSubBuffer> attribs);


        void bind_per_instance_attrib_buffers(core::Span<AttribSubBuffer> per_instance);

        // proxies from _cmd_buffer
        CmdBufferRegion region(const char* name, const math::Vec4& color = math::Vec4());

        VkCommandBuffer vk_cmd_buffer() const;

        // Statefull stuff
        const Viewport& viewport() const;
        void set_viewport(const Viewport& vp);
        void set_scissor(const math::Vec2i& offset, const math::Vec2ui& size);


        template<typename T>
        void keep_alive(T&& t);

    private:
        friend class CmdBufferRecorder;

        RenderPassRecorder(CmdBufferRecorder& cmd_buffer, const Viewport& viewport);

        CmdBufferRecorder& _cmd_buffer;
        Viewport _viewport;
        VkDescriptorSet _main_descriptor_set = {};

        struct {
            const MeshBufferData* mesh_buffer_data = nullptr;
            const MaterialTemplate* material = nullptr;
            VkPipelineLayout pipeline_layout = {};
        } _cache;
};

class CmdBufferRecorder final : NonCopyable {

    using SrcCopySubBuffer = SubBuffer<BufferUsage::TransferSrcBit, MemoryType::DontCare>;
    using DstCopySubBuffer = SubBuffer<BufferUsage::TransferDstBit, MemoryType::DontCare>;
    using SrcCopyImage = ImageView<ImageUsage::TransferSrcBit>;
    using DstCopyImage = ImageView<ImageUsage::TransferDstBit>;

    public:
        CmdBufferRecorder(CmdBufferRecorder&& other);
        CmdBufferRecorder& operator=(CmdBufferRecorder&& other);

        ~CmdBufferRecorder();

        VkCommandBuffer vk_cmd_buffer() const;
        ResourceFence resource_fence() const;

        CmdBufferRegion region(const char* name, const math::Vec4& color = math::Vec4());

        bool is_inside_renderpass() const;
        RenderPassRecorder bind_framebuffer(const Framebuffer& framebuffer);

        void dispatch(const ComputeProgram& program, const math::Vec3ui& size, core::Span<DescriptorSetBase> descriptor_sets);
        void dispatch_size(const ComputeProgram& program, const math::Vec3ui& size, core::Span<DescriptorSetBase> descriptor_sets);
        void dispatch_size(const ComputeProgram& program, const math::Vec2ui& size, core::Span<DescriptorSetBase> descriptor_sets);


        void barriers(core::Span<BufferBarrier> buffers, core::Span<ImageBarrier> images);
        void barriers(core::Span<BufferBarrier> buffers);
        void barriers(core::Span<ImageBarrier> images);

        void full_barrier();



        Y_TODO(Const all this)
        void barriered_copy(const ImageBase& src,  const ImageBase& dst);
        void copy(SrcCopySubBuffer src, DstCopySubBuffer dst);
        //void copy(const SrcCopyImage& src,  const DstCopyImage& dst);
        void blit(const SrcCopyImage& src,  const DstCopyImage& dst);



        template<typename T>
        void keep_alive(T&& t) {
            _data->keep_alive(y_fwd(t));
        }

    private:
        friend class ImageBase;

        void transition_image(ImageBase& image, VkImageLayout src, VkImageLayout dst);

    private:
        friend class RenderPassRecorder;
        friend class CmdBufferPool;
        friend class CmdQueue;

        CmdBufferRecorder() = default;
        CmdBufferRecorder(CmdBufferData* data);

        void swap(CmdBufferRecorder& other);

        void end_renderpass();
        void check_no_renderpass() const;

        CmdBufferData* _data = nullptr;
        // this could be in RenderPassRecorder, but putting it here makes erroring easier
        const RenderPass* _render_pass = nullptr;
};


template<typename T>
void RenderPassRecorder::keep_alive(T&& t) {
   _cmd_buffer.keep_alive(y_fwd(t));
}

}

#endif // YAVE_GRAPHICS_COMMANDS_CMDBUFFERRECORDER_H

