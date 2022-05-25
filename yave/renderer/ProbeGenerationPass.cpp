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

#include "ProbeGenerationPass.h"

#include "ShadowMapPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/shaders/ComputeProgram.h>


namespace yave {

const math::Vec3ui screen_division = math::Vec3ui(16, 16, 1);

static math::Vec2ui compute_probe_count(const math::Vec2ui& size) {
    math::Vec2ui probe_count;
    for(usize i = 0; i != 2; ++i) {
        probe_count[i] = size[i] / screen_division[i] + !!(size[i] % screen_division[i]);
    }
    return probe_count;
}

ProbeGenerationPass ProbeGenerationPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer) {
    const math::Vec2ui gbuffer_size = framegraph.image_size(gbuffer.depth);
    const math::Vec2ui probe_count = compute_probe_count(gbuffer_size);
    const usize total_probe_count = probe_count.x() * probe_count.y();

    FrameGraphPassBuilder builder = framegraph.add_pass("Probe placement pass");

    const auto probe_buffer = builder.declare_typed_buffer<math::Vec4>(total_probe_count);

    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(gbuffer.scene_pass.camera_buffer);
    builder.add_storage_output(probe_buffer);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const auto& program = device_resources()[DeviceResources::GenerateProbesProgram];
        y_debug_assert(program.local_size() == screen_division);
        recorder.dispatch(program, math::Vec3ui(probe_count, 1), {self->descriptor_sets()[0]});
    });

    ProbeGenerationPass pass;
    pass.probe_screen_size = screen_division.to<2>();
    pass.probe_count = probe_count;
    pass.probe_buffer = probe_buffer;
    return pass;
}

}
