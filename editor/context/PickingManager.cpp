/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include "PickingManager.h"
#include "EditorContext.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/framegraph/FrameGraphResourcePool.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/graphics/shaders/ComputeProgram.h>
#include <yave/graphics/device/Queue.h>

#include <editor/renderer/EditorEntityPass.h>
#include <editor/renderer/ScenePickingPass.h>

namespace editor {

bool PickingManager::PickingData::hit() const {
    return depth > 0.0f;
}

PickingManager::PickingManager(ContextPtr ctx) :
        ContextLinked(ctx),
        _buffer(device(), 1) {
}

PickingManager::PickingData PickingManager::pick_sync(const SceneView& scene_view, const math::Vec2& uv, const math::Vec2ui& size) {
    y_profile();

    FrameGraph framegraph(std::make_shared<FrameGraphResourcePool>(device()));

    Y_TODO(Take editor renderer settings into account for picking)
    const ScenePickingPass scene_pass = ScenePickingPass::create(context(), framegraph, scene_view, size);
    const EditorEntityPass entity_pass = EditorEntityPass::create(context(), framegraph, scene_view, scene_pass.depth, scene_pass.id, true);

    {
        FrameGraphPassBuilder builder = framegraph.add_pass("Picking readback pass");

        builder.add_uniform_input(entity_pass.depth);
        builder.add_uniform_input(entity_pass.id);
        builder.add_descriptor_binding(Descriptor(_buffer));

        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            const auto& program = context()->resources()[EditorResources::PickingProgram];
            recorder.dispatch(program, math::Vec3ui(1), {self->descriptor_sets()[0]}, uv);
        });
    }

    CmdBufferRecorder recorder = create_disposable_cmd_buffer(device());
    std::move(framegraph).render(recorder);
    graphic_queue(device()).submit<SyncPolicy::Sync>(std::move(recorder));

    const ReadBackData read_back = TypedMapping(_buffer)[0];
    const float depth = read_back.depth;

    auto inv_matrix = scene_view.camera().inverse_matrix();
    const math::Vec4 p = inv_matrix * math::Vec4(uv * 2.0f - 1.0f, depth, 1.0f);

    const PickingData data{
            p.to<3>() / p.w(),
            depth,
            uv,
            read_back.id
        };

    //log_msg(fmt("picked: % (depth: %, id: %)", data.world_pos, data.depth, data.entity_index));
    return data;
}

}

