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

#include "AmbientPass.h"

#include "AOPass.h"
#include "RTGIPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/scene/Scene.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/graphics/images/IBLProbe.h>

#include <yave/components/SkyLightComponent.h>
#include <yave/ecs/EntityWorld.h>


namespace yave {

static FrameGraphMutableImageId ambient_pass(FrameGraph& framegraph,
                                             const GBufferPass& gbuffer,
                                             FrameGraphImageId in_lit,
                                             FrameGraphImageId ao_or_gi,
                                             bool is_gi) {

    const math::Vec2ui size = framegraph.image_size(in_lit);
    const SceneVisibility& visibility = *gbuffer.scene_pass.visibility.visible;

    auto [ibl_probe, intensity, sky] = visibility.ibl_probe();
    const Texture& black = *device_resources()[DeviceResources::BlackTexture];
    const Texture& white = *device_resources()[DeviceResources::WhiteTexture];

    struct Params {
        u32 display_sky;
        float ibl_intensity;
    } params {
        sky ? 1u : 0u,
        intensity
    };


    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Ambient pass");

    const auto lit = builder.declare_copy(in_lit);

    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(gbuffer.color);
    builder.add_uniform_input(gbuffer.normal);
    builder.add_uniform_input_with_default(ao_or_gi, Descriptor(is_gi ? black : white));
    builder.add_external_input(*ibl_probe);
    builder.add_external_input(Descriptor(device_resources().brdf_lut(), SamplerType::LinearClamp));
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_storage_output(lit);
    builder.add_inline_input(params);

    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const auto& program = device_resources()[is_gi ? DeviceResources::DeferredAmbientGIProgram : DeviceResources::DeferredAmbientAOProgram];
        recorder.dispatch_threads(program, size, self->descriptor_set());
    });

    return lit;
}

AmbientPass AmbientPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId lit, const RTGIPass& gi) {
    return {ambient_pass(framegraph, gbuffer, lit, gi.gi, true)};
}

AmbientPass AmbientPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId lit, const AOPass& ao) {
    return {ambient_pass(framegraph, gbuffer, lit, ao.ao, false)};
}

}

