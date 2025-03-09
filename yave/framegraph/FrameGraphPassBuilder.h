/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#ifndef YAVE_FRAMEGRAPH_FrameGraphPassBuilderBase_H
#define YAVE_FRAMEGRAPH_FrameGraphPassBuilderBase_H

#include "FrameGraphResourceId.h"

#include <yave/graphics/barriers/PipelineStage.h>
#include <yave/graphics/descriptors/Descriptor.h>
#include <yave/graphics/images/SamplerType.h>
#include <yave/graphics/images/ImageUsage.h>
#include <yave/graphics/images/SamplerType.h>
#include <yave/graphics/buffers/BufferUsage.h>

namespace yave {

class FrameGraphPassBuilderBase {

    public:
        // We are using screen textures 90% of the time, so clamp makes sense as a default
        const SamplerType default_samplers[2] = {
            SamplerType::PointClamp,
            SamplerType::LinearClamp,
        };

        using render_func = std::function<void(RenderPassRecorder&, const FrameGraphPass*)>;
        using compute_render_func = std::function<void(CmdBufferRecorder&, const FrameGraphPass*)>;

        FrameGraphMutableImageId declare_image(ImageFormat format, const math::Vec2ui& size, u32 mips = 1);
        FrameGraphMutableVolumeId declare_volume(ImageFormat format, const math::Vec3ui& size);
        FrameGraphMutableBufferId declare_buffer(u64 byte_size);

        FrameGraphMutableImageId declare_copy(FrameGraphImageId src);
        FrameGraphMutableBufferId declare_copy(FrameGraphBufferId src);

        template<typename T>
        FrameGraphMutableTypedBufferId<T> declare_typed_buffer(usize size = 1) {
            return FrameGraphMutableTypedBufferId<T>::from_untyped(declare_buffer(sizeof(T) * std::max(usize(1u), size)));
        }

        void add_input_usage(FrameGraphImageId res, ImageUsage usage);
        void add_input_usage(FrameGraphBufferId res, BufferUsage usage);

        void add_output_usage(FrameGraphMutableImageId res, ImageUsage usage);
        void add_output_usage(FrameGraphBufferId res, BufferUsage usage);

        void add_depth_output(FrameGraphMutableImageId res);
        void add_color_output(FrameGraphMutableImageId res, u32 mip = 0);

        void add_storage_output(FrameGraphMutableImageId res,   PipelineStage stage = PipelineStage::None, i32 ds_index = -1);
        void add_storage_output(FrameGraphMutableVolumeId res,  PipelineStage stage = PipelineStage::None, i32 ds_index = -1);
        void add_storage_output(FrameGraphMutableBufferId res,  PipelineStage stage = PipelineStage::None, i32 ds_index = -1);

        void add_storage_input(FrameGraphBufferId res,      PipelineStage stage = PipelineStage::None, i32 ds_index = -1);
        void add_storage_input(FrameGraphVolumeId res,      PipelineStage stage = PipelineStage::None, i32 ds_index = -1);
        void add_storage_input(FrameGraphImageId res,       PipelineStage stage = PipelineStage::None, i32 ds_index = -1);

        void add_uniform_input(FrameGraphBufferId res,      PipelineStage stage = PipelineStage::None, i32 ds_index = -1);
        void add_uniform_input(FrameGraphVolumeId res,      PipelineStage stage = PipelineStage::None, i32 ds_index = -1);
        void add_uniform_input(FrameGraphImageId res,       PipelineStage stage = PipelineStage::None, i32 ds_index = -1);

        void add_uniform_input(FrameGraphVolumeId res,  SamplerType sampler, PipelineStage stage = PipelineStage::None, i32 ds_index = -1);
        void add_uniform_input(FrameGraphImageId res,   SamplerType sampler, PipelineStage stage = PipelineStage::None, i32 ds_index = -1);

        void add_uniform_input_with_default(FrameGraphImageId res, Descriptor desc, PipelineStage stage = PipelineStage::None, i32 ds_index = -1);

        void add_inline_input(InlineDescriptor desc, i32 ds_index = -1);
        void add_external_input(Descriptor desc, PipelineStage stage = PipelineStage::None, i32 ds_index = -1);

        void add_attrib_input(FrameGraphBufferId res, PipelineStage stage = PipelineStage::VertexInputBit);
        void add_index_input(FrameGraphBufferId res, PipelineStage stage = PipelineStage::VertexInputBit);

        void add_indrect_input(FrameGraphBufferId res, PipelineStage stage = PipelineStage::IndirectBit);

        void clear_before_pass(FrameGraphMutableImageId res);

        template<typename T>
        void add_inline_input(const T& t, i32 ds_index = -1) {
            add_inline_input(InlineDescriptor(t), ds_index);
        }

        template<typename T>
        void map_buffer(FrameGraphMutableTypedBufferId<T> res) {
            map_buffer_internal(res);
        }

        template<typename T>
        void map_buffer(FrameGraphMutableTypedBufferId<T> res, const T& data) {
            static_assert(sizeof(T) % sizeof(u32) == 0);
            map_buffer_internal(res, {reinterpret_cast<const u32*>(&data), sizeof(T) / sizeof(u32)});
        }

        void add_descriptor_binding(Descriptor desc, i32 ds_index = -1);
        i32 next_descriptor_set_index() const;

    protected:
        FrameGraphPassBuilderBase(FrameGraphPass* pass, PipelineStage default_stage = PipelineStage::AllShadersBit);

        void set_render_func(render_func&& func);
        void set_compute_render_func(compute_render_func&& func);

    private:
        void add_to_pass(FrameGraphImageId res, ImageUsage usage, bool is_written, PipelineStage stage);
        void add_to_pass(FrameGraphVolumeId res, ImageUsage usage, bool is_written, PipelineStage stage);
        void add_to_pass(FrameGraphBufferId res, BufferUsage usage, bool is_written, PipelineStage stage);

        void add_descriptor_binding(FrameGraphDescriptorBinding binding, i32 ds_index);

        void map_buffer_internal(FrameGraphMutableBufferId res, FrameGraphInlineBlock block = {});

        PipelineStage or_default(PipelineStage stage) const;

        FrameGraph* parent() const;

        FrameGraphPass* _pass = nullptr;
        PipelineStage _default_stage = PipelineStage::AllShadersBit;
};


class FrameGraphPassBuilder : public FrameGraphPassBuilderBase {
    public:
        template<typename F>
        void set_render_func(F&& func) {
            FrameGraphPassBuilderBase::set_render_func(render_func(std::move(func)));
        }

    private:
        friend class FrameGraph;

        FrameGraphPassBuilder(FrameGraphPass* pass) : FrameGraphPassBuilderBase(pass, PipelineStage::FragmentBit) {
        }
};

class FrameGraphComputePassBuilder : public FrameGraphPassBuilderBase {
    public:
        template<typename F>
        void set_render_func(F&& func) {
            FrameGraphPassBuilderBase::set_compute_render_func(compute_render_func(std::move(func)));
        }

    private:
        friend class FrameGraph;

        FrameGraphComputePassBuilder(FrameGraphPass* pass) : FrameGraphPassBuilderBase(pass, PipelineStage::ComputeBit) {
        }
};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHBUILDER_H

