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

#include "RaytracingPass.h"
#include "GBufferPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/graphics/device/MaterialAllocator.h>
#include <yave/graphics/images/TextureLibrary.h>
#include <yave/components/SkyLightComponent.h>


namespace yave {

RaytracingPass RaytracingPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, const math::Vec2ui& size) {
    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Raytracing");

    const SceneView& scene_view = gbuffer.scene_pass.scene_view;
    const SceneVisibility& visibility = *gbuffer.scene_pass.visibility.visible;

    const IBLProbe* ibl_probe = visibility.sky_light ? visibility.sky_light->component.probe().get() : nullptr;
    const TLAS& tlas = scene_view.scene()->tlas();

    const auto raytraced = builder.declare_image(VK_FORMAT_R8G8B8A8_UNORM, size);

    builder.add_descriptor_binding(Descriptor(tlas));
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(gbuffer.color);
    builder.add_uniform_input(gbuffer.normal);
    builder.add_external_input(Descriptor(material_allocator().material_buffer()));
    builder.add_external_input(ibl_probe ? *ibl_probe : *device_resources().empty_probe());
    builder.add_storage_output(raytraced);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        const std::array<DescriptorSetProxy, 2> desc_sets = {
            self->descriptor_set(),
            texture_library().descriptor_set()
        };
        recorder.dispatch_threads(device_resources()[DeviceResources::RTProgram], size, desc_sets);
    });

    RaytracingPass pass;
    pass.raytraced = raytraced;
    return pass;
}


}

