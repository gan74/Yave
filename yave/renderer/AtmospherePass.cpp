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

#include "AtmospherePass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/AtmosphereComponent.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/ecs/EntityWorld.h>

#include <yave/graphics/shader_structs.h>

#include <y/core/Chrono.h>

namespace yave {

AtmospherePass AtmospherePass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId lit, const AtmosphereSettings&) {
    const Scene* scene = gbuffer.scene_pass.scene_view.scene();

    const AtmosphereObject* atmo_object = scene->atmosphere();
    if(!atmo_object) {
        AtmospherePass pass;
        pass.lit = lit;
        return pass;
    }

    const AtmosphereComponent& atmosphere = atmo_object->component;
    const DirectionalLightComponent& sun = atmo_object->sun;

    const math::Vec4 params(
        sun.color() * sun.intensity(),
        atmosphere.density_falloff() / 1000.0f
    );

    FrameGraphPassBuilder builder = framegraph.add_pass("Atmosphere pass");

    const auto atmo = builder.declare_copy(lit);

    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(lit);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_color_output(atmo);
    builder.add_inline_input(InlineDescriptor(params));
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        const auto* material = device_resources()[DeviceResources::AtmosphereMaterialTemplate];
        render_pass.bind_material_template(material, self->descriptor_sets());
        render_pass.draw_array(3);
    });

    AtmospherePass pass;
    pass.lit = atmo;
    return pass;
}

}

