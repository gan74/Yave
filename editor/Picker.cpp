/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include "Picker.h"

#include <editor/EditorResources.h>
#include <editor/renderer/EditorPass.h>
#include <editor/renderer/IdBufferPass.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/framegraph/FrameGraphResourcePool.h>
#include <yave/graphics/commands/CmdQueue.h>

#include <yave/graphics/shaders/ComputeProgram.h>

namespace editor {

bool PickingResult::hit() const {
    return depth > 0.0f;
}


PickingResult Picker::pick_sync(const SceneView& scene_view, const math::Vec2& uv, const math::Vec2ui& size) {
    y_profile();

    struct ReadBackData {
        const float depth;
        const u32 id;
    };

    using ReadBackBuffer = TypedBuffer<ReadBackData, BufferUsage::StorageBit, MemoryType::CpuVisible>;

    ReadBackBuffer buffer(1);

    FrameGraph framegraph(std::make_shared<FrameGraphResourcePool>());

    Y_TODO(Take editor renderer settings into account for picking)
    const IdBufferPass scene_pass = IdBufferPass::create(framegraph, scene_view, size);
    const EditorPass entity_pass = EditorPass::create(framegraph, scene_view, scene_pass.depth, FrameGraphImageId(), scene_pass.id);

    {
        FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Picking readback pass");

        builder.add_uniform_input(entity_pass.depth);
        builder.add_uniform_input(entity_pass.id);
        builder.add_descriptor_binding(Descriptor(buffer));

        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            const auto& program = resources()[EditorResources::PickingProgram];

            const auto uv_set = DescriptorSet(InlineDescriptor(uv));
            const std::array<DescriptorSetBase, 2> descriptor_sets = {self->descriptor_sets()[0], uv_set};
            recorder.dispatch(program, math::Vec3ui(1), descriptor_sets);
        });
    }

    CmdBufferRecorder recorder = create_disposable_cmd_buffer();
    framegraph.render(recorder);
    recorder.submit().wait();

    const ReadBackData read_back = buffer.map(MappingAccess::ReadOnly)[0];
    const float depth = read_back.depth;

    auto inv_matrix = scene_view.camera().inverse_matrix();
    const math::Vec4 p = inv_matrix * math::Vec4(uv * 2.0f - 1.0f, depth, 1.0f);

    const PickingResult result {
            p.to<3>() / p.w(),
            depth,
            uv,
            read_back.id
        };

    //log_msg(fmt("picked: {} (depth: {}, id: {})", result.world_pos, result.depth, result.entity_index));
    return result;
}

}

