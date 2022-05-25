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

#include "ProbeFillingPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/shaders/ComputeProgram.h>

namespace yave {

ProbeFillingPass ProbeFillingPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, const VPLGenerationPass& vpls) {
    const math::Vec2ui probe_size(32);

    const ProbeGenerationPass probes = ProbeGenerationPass::create(framegraph, gbuffer);

    const math::Vec2ui atlas_size = probe_size * probes.probe_count;

    FrameGraphPassBuilder builder = framegraph.add_pass("Probe generation pass");

    const usize vpl_count = framegraph.buffer_size(vpls.vpl_buffer);

    const auto probe_depth_atlas = builder.declare_image(VK_FORMAT_D32_SFLOAT, atlas_size);
    const auto probe_atlas = builder.declare_image(VK_FORMAT_R16G16B16A16_SFLOAT, atlas_size);

    builder.add_depth_output(probe_depth_atlas);
    builder.add_color_output(probe_atlas);
    builder.add_storage_input(vpls.vpl_buffer);
    builder.add_storage_input(probes.probe_buffer);
    builder.add_inline_input(InlineDescriptor(probes.probe_count));
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        auto render_pass = recorder.bind_framebuffer(self->framebuffer());
        const MaterialTemplate* material = device_resources()[DeviceResources::VPLSplatMaterialTemplate];
        render_pass.bind_material_template(material, self->descriptor_sets()[0]);
        render_pass.draw_array(vpl_count, probes.probe_count.x() * probes.probe_count.y());
    });


    ProbeFillingPass pass;
    pass.probes = probes;
    pass.probe_atlas = probe_atlas;
    return pass;
}

}

