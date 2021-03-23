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
#ifndef YAVE_FRAMEGRAPH_FRAMEGRAPHPASSBUILDER_H
#define YAVE_FRAMEGRAPH_FRAMEGRAPHPASSBUILDER_H

#include "FrameGraphResourceId.h"

#include <yave/graphics/barriers/PipelineStage.h>
#include <yave/graphics/images/SamplerType.h>
#include <yave/graphics/images/ImageUsage.h>
#include <yave/graphics/images/SamplerType.h>
#include <yave/graphics/buffers/BufferUsage.h>

namespace yave {

class FrameGraphPassBuilder {
    public:
        // We are using screen textures 90% of the time, so clamp makes sense as a default
        const SamplerType default_samplers[2] = {
            SamplerType::PointClamp,
            SamplerType::LinearClamp,
        };

        using render_func = std::function<void(CmdBufferRecorder&, const FrameGraphPass*)>;

        FrameGraphMutableImageId declare_image(ImageFormat format, const math::Vec2ui& size);
        FrameGraphMutableBufferId declare_buffer(usize byte_size);

        FrameGraphMutableImageId declare_copy(FrameGraphImageId src);
        //FrameGraphMutableBufferId declare_copy(FrameGraphBufferId src);

        template<typename T>
        FrameGraphMutableTypedBufferId<T> declare_typed_buffer(usize size = 1) {
            return FrameGraphMutableTypedBufferId<T>::from_untyped(declare_buffer(sizeof(T) * size));
        }

        void add_image_input_usage(FrameGraphImageId res, ImageUsage usage);

        void add_depth_output(FrameGraphMutableImageId res);
        void add_color_output(FrameGraphMutableImageId res);

        void add_storage_output(FrameGraphMutableImageId res, usize ds_index = 0, PipelineStage stage = PipelineStage::ComputeBit);
        void add_storage_output(FrameGraphMutableBufferId res, usize ds_index = 0, PipelineStage stage = PipelineStage::ComputeBit);

        void add_storage_input(FrameGraphBufferId res, usize ds_index = 0, PipelineStage stage = PipelineStage::AllShadersBit);
        void add_storage_input(FrameGraphImageId res, usize ds_index = 0, PipelineStage stage = PipelineStage::AllShadersBit);
        void add_uniform_input(FrameGraphBufferId res, usize ds_index = 0, PipelineStage stage = PipelineStage::AllShadersBit);
        void add_uniform_input(FrameGraphImageId res, usize ds_index = 0, PipelineStage stage = PipelineStage::AllShadersBit);
        void add_uniform_input(FrameGraphImageId res, SamplerType sampler, usize ds_index = 0, PipelineStage stage = PipelineStage::AllShadersBit);
        void add_uniform_input_with_default(FrameGraphImageId res, Descriptor desc, usize ds_index = 0, PipelineStage stage = PipelineStage::AllShadersBit);

        void add_inline_input(InlineDescriptor desc, usize ds_index = 0);
        void add_external_input(Descriptor desc, usize ds_index = 0, PipelineStage stage = PipelineStage::AllShadersBit);

        void add_attrib_input(FrameGraphBufferId res, PipelineStage stage = PipelineStage::VertexInputBit);
        void add_index_input(FrameGraphBufferId res, PipelineStage stage = PipelineStage::VertexInputBit);

        template<typename T>
        void map_update(FrameGraphMutableTypedBufferId<T> res) {
            set_cpu_visible(res);
        }

        template<typename F>
        void set_render_func(F&& func) {
            set_render_func(render_func(std::move(func)));
        }

        void set_render_func(render_func&& func);

        void add_descriptor_binding(Descriptor bind, usize ds_index = 0);
        usize next_descriptor_set_index();

    private:
        friend class FrameGraph;

        FrameGraphPassBuilder(FrameGraphPass* pass);

        void add_to_pass(FrameGraphImageId res, ImageUsage usage, bool is_written, PipelineStage stage);
        void add_to_pass(FrameGraphBufferId res, BufferUsage usage, bool is_written, PipelineStage stage);

        void add_uniform(FrameGraphDescriptorBinding binding, usize ds_index);

        void set_cpu_visible(FrameGraphMutableBufferId res);

        FrameGraph* parent() const;

        FrameGraphPass* _pass = nullptr;
};

}

#endif // YAVE_FRAMEGRAPH_FRAMEGRAPHBUILDER_H

