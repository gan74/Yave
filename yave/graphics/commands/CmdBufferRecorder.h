/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#include <yave/graphics/buffers/Buffer.h>

namespace yave {

class CmdBufferRegion : NonCopyable {
    public:
        CmdBufferRegion() = default;
        ~CmdBufferRegion();

        CmdBufferRegion(CmdBufferRegion&& other);
        CmdBufferRegion& operator=(CmdBufferRegion&& other);

        void swap(CmdBufferRegion& other);

    private:
        friend class CmdBufferRecorderBase;

        CmdBufferRegion(const CmdBufferRecorderBase& cmd_buffer, CmdTimingRecorder* time_rec, CmdQueue* queue, const char* name, const math::Vec4& color);

        VkCommandBuffer _buffer = {};
        CmdTimingRecorder* _time_recorder = nullptr;

#ifdef YAVE_GPU_PROFILING
        u64 _profiling_scope[3] = {};
#endif
};

class RenderPassRecorder final : NonMovable {
    public:
        ~RenderPassRecorder();

        // specific
        void bind_material_template(const MaterialTemplate* material_template, core::Span<DescriptorSetBase> sets, bool bind_main_ds = false);

        void set_main_descriptor_set(DescriptorSetBase ds_set);

        void draw(const MeshDrawData& draw_data, u32 instance_count = 1, u32 instance_index = 0);

        void draw(const VkDrawIndexedIndirectCommand& indirect);
        void draw(const VkDrawIndirectCommand& indirect);

        void draw_indexed(usize index_count);
        void draw_array(usize vertex_count, usize instance_count = 1);

        void bind_mesh_buffers(const MeshDrawBuffers& mesh_buffers);
        void bind_index_buffer(IndexSubBuffer indices);
        void bind_attrib_buffers(core::Span<AttribSubBuffer> attribs);


        void bind_per_instance_attrib_buffers(core::Span<AttribSubBuffer> per_instance);

        // proxies from _cmd_buffer
        CmdBufferRegion region(const char* name, CmdTimingRecorder* time_rec = nullptr, const math::Vec4& color = math::Vec4());

        VkCommandBuffer vk_cmd_buffer() const;

        // Statefull stuff
        const Viewport& viewport() const;
        void set_viewport(const Viewport& vp);
        void set_scissor(const math::Vec2i& offset, const math::Vec2ui& size);

    private:
        friend class CmdBufferRecorder;

        RenderPassRecorder(CmdBufferRecorder& cmd_buffer, const Viewport& viewport);

        CmdBufferRecorder& _cmd_buffer;
        Viewport _viewport;
        VkDescriptorSet _main_descriptor_set = {};

        struct {
            const MeshDrawBuffers* mesh_buffers = nullptr;
            const MaterialTemplate* material = nullptr;
            VkPipelineLayout pipeline_layout = {};
        } _cache;
};


class CmdBufferRecorderBase : NonMovable {
    public:
        using SrcCopySubBuffer = SubBuffer<BufferUsage::TransferSrcBit, MemoryType::DontCare>;
        using DstCopySubBuffer = SubBuffer<BufferUsage::TransferDstBit, MemoryType::DontCare>;
        using SrcCopyImage = ImageView<ImageUsage::TransferSrcBit>;
        using DstCopyImage = ImageView<ImageUsage::TransferDstBit>;

        ~CmdBufferRecorderBase();

        CmdQueue* queue() const;

        VkCommandBuffer vk_cmd_buffer() const;
        ResourceFence resource_fence() const;

        bool is_inside_renderpass() const;

        CmdBufferRegion region(const char* name, CmdTimingRecorder* time_rec = nullptr, const math::Vec4& color = math::Vec4());

        void barriers(core::Span<BufferBarrier> buffers, core::Span<ImageBarrier> images);
        void barriers(core::Span<BufferBarrier> buffers);
        void barriers(core::Span<ImageBarrier> images);

        void full_barrier();

        // Clean these
        void copy(const ImageBase& src,  const ImageBase& dst);
        void clear(const ImageBase& dst);

        void unbarriered_copy(SrcCopySubBuffer src, DstCopySubBuffer dst);

    protected:
        void dispatch(const ComputeProgram& program, const math::Vec3ui& size, core::Span<DescriptorSetBase> descriptor_sets);
        void dispatch_size(const ComputeProgram& program, const math::Vec3ui& size, core::Span<DescriptorSetBase> descriptor_sets);
        void dispatch_size(const ComputeProgram& program, const math::Vec2ui& size, core::Span<DescriptorSetBase> descriptor_sets);

        TimelineFence submit();
        void submit_async();

    protected:
        friend class RenderPassRecorder;

        CmdBufferRecorderBase() = default;
        CmdBufferRecorderBase(CmdBufferData* data);

        void swap(CmdBufferRecorderBase& other);

        void end_renderpass();
        void check_no_renderpass() const;


        CmdBufferData* _data = nullptr;
        // this could be in RenderPassRecorder, but putting it here makes erroring easier
        const RenderPass* _render_pass = nullptr;
};



#define YAVE_GENERATE_CMD_BUFFER_MEMBERS(CmdBufferType)                                                     \
    public:                                                                                                 \
        CmdBufferType(CmdBufferType&& other) { swap(other); }                                               \
        CmdBufferType& operator=(CmdBufferType&& other) { swap(other); return *this; }                      \
        using CmdBufferRecorderBase::submit;                                                                \
    private:                                                                                                \
        friend class CmdBufferPool;                                                                         \
        CmdBufferType() = default;                                                                          \
        CmdBufferType(CmdBufferData* data) : CmdBufferRecorderBase(data) {}



class TransferCmdBufferRecorder final : public CmdBufferRecorderBase {
    YAVE_GENERATE_CMD_BUFFER_MEMBERS(TransferCmdBufferRecorder)

    public:
        using CmdBufferRecorderBase::submit_async;
};

class ComputeCmdBufferRecorder final : public CmdBufferRecorderBase {
    YAVE_GENERATE_CMD_BUFFER_MEMBERS(ComputeCmdBufferRecorder)

    public:
        using CmdBufferRecorderBase::submit_async;

        using CmdBufferRecorderBase::dispatch;
        using CmdBufferRecorderBase::dispatch_size;
};

class CmdBufferRecorder final : public CmdBufferRecorderBase {
    YAVE_GENERATE_CMD_BUFFER_MEMBERS(CmdBufferRecorder)

    friend class CmdQueue; // Needed for present

    public:

        using CmdBufferRecorderBase::dispatch;
        using CmdBufferRecorderBase::dispatch_size;

        RenderPassRecorder bind_framebuffer(const Framebuffer& framebuffer);

        void execute(CmdBufferRecorder&& other);
};

static_assert(sizeof(TransferCmdBufferRecorder) == sizeof(CmdBufferRecorderBase));
static_assert(sizeof(ComputeCmdBufferRecorder) == sizeof(CmdBufferRecorderBase));
static_assert(sizeof(CmdBufferRecorder) == sizeof(CmdBufferRecorderBase));

#undef YAVE_GENERATE_CMD_BUFFER_MEMBERS

}

#endif // YAVE_GRAPHICS_COMMANDS_CMDBUFFERRECORDER_H

