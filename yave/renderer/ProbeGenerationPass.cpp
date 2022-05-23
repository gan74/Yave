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
const math::Vec2ui probe_size(32);

static math::Vec2ui compute_probe_count(const math::Vec2ui& size) {
    math::Vec2ui probe_count;
    for(usize i = 0; i != 2; ++i) {
        probe_count[i] = size[i] / screen_division[i] + !!(size[i] % screen_division[i]);
    }
    return probe_count;
}

ProbeGenerationPass ProbeGenerationPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, const VPLGenerationPass& vpls) {
    const math::Vec2ui gbuffer_size = framegraph.image_size(gbuffer.depth);
    const math::Vec2ui probe_count = compute_probe_count(gbuffer_size);
    const math::Vec2ui atlas_size = probe_size * probe_count;

    FrameGraphMutableTypedBufferId<math::Vec3> probe_buffer;

    {
        FrameGraphPassBuilder builder = framegraph.add_pass("Probe placement pass");

        probe_buffer = builder.declare_typed_buffer<math::Vec3>(probe_count.x() * probe_count.y());

        builder.add_uniform_input(gbuffer.depth);
        builder.add_uniform_input(gbuffer.scene_pass.camera_buffer, 0, PipelineStage::ComputeBit);
        builder.add_storage_output(probe_buffer);
        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            const auto& program = device_resources()[DeviceResources::ProbePlacementProgram];
            y_debug_assert(program.local_size() == screen_division);
            recorder.dispatch(program, math::Vec3ui(probe_count, 1), {self->descriptor_sets()[0]});
        });
    }

    FrameGraphPassBuilder builder = framegraph.add_pass("Probe generation pass");

    const usize vpl_count = framegraph.buffer_size(vpls.vpl_buffer);

    const auto probe_depth_atlas = builder.declare_image(VK_FORMAT_D32_SFLOAT, atlas_size);
    const auto probe_atlas = builder.declare_image(VK_FORMAT_R16G16B16A16_SFLOAT, atlas_size);

    builder.add_depth_output(probe_depth_atlas);
    builder.add_color_output(probe_atlas);
    builder.add_storage_input(vpls.vpl_buffer);
    builder.add_storage_input(probe_buffer);
    builder.add_inline_input(InlineDescriptor(probe_count));
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        auto render_pass = recorder.bind_framebuffer(self->framebuffer());
        const MaterialTemplate* material = device_resources()[DeviceResources::VPLSplatMaterialTemplate];
        render_pass.bind_material_template(material, self->descriptor_sets()[0]);
        render_pass.draw_array(vpl_count, probe_count.x() * probe_count.y());
    });


    ProbeGenerationPass pass;
    pass.gbuffer = gbuffer;
    pass.probe_screen_size = screen_division.to<2>();
    pass.probe_count = probe_count;
    pass.probe_atlas = probe_atlas;
    return pass;
}

}

