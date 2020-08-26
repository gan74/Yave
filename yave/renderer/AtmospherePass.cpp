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

#include "AtmospherePass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/ecs/EntityWorld.h>

#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/AtmosphereComponent.h>

#include <yave/meshes/StaticMesh.h>

#include <y/core/Chrono.h>

namespace yave {

struct AtmosphereData {
    float planet_radius;
    float atmosphere_height;
    float radius;
    float density_falloff;

    math::Vec3 center;
    float scattering_strength;

    math::Vec3 light_dir;
    u32 padding_0;

    math::Vec3 wavelengths;
    u32 padding_1;
};

static const DirectionalLightComponent* find_sun(const SceneView& scene) {
    for(const auto& sun : scene.world().components<DirectionalLightComponent>()) {
        return &sun;
    }

    return nullptr;
}

static const AtmosphereComponent* find_atmosphere_component(const SceneView& scene) {
    for(const auto& atmo : scene.world().components<AtmosphereComponent>()) {
        return &atmo;
    }

    return nullptr;
}

AtmospherePass AtmospherePass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId lit) {
    const AtmosphereComponent* atmosphere = find_atmosphere_component(gbuffer.scene_pass.scene_view);
    const DirectionalLightComponent* sun = find_sun(gbuffer.scene_pass.scene_view);

    if(!atmosphere || !sun) {
        AtmospherePass pass;
        pass.lit = lit;
        return pass;
    }

    /*const float time = core::Chrono::program().to_secs() / 5.0;
    const math::Vec3 sun_dir(0.0f, std::cos(time), std::abs(std::sin(time)));*/

    AtmosphereData params {
        atmosphere->planet_radius,
        atmosphere->atmosphere_height,
        atmosphere->planet_radius + atmosphere->atmosphere_height,
        atmosphere->density_falloff,

        math::Vec3(0.0f, 0.0f, -atmosphere->planet_radius - 0.001f),
        atmosphere->scattering_strength,

        -sun->direction().normalized(),
        0,

        atmosphere->wavelengths,
        0
    };

    FrameGraphPassBuilder builder = framegraph.add_pass("Atmospheric pass");

    const auto atmo = builder.declare_copy(lit);

    const auto atmo_params = builder.declare_typed_buffer<AtmosphereData>();

    builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::FragmentBit);
    builder.add_uniform_input(gbuffer.scene_pass.camera_buffer, 0, PipelineStage::FragmentBit);
    builder.add_uniform_input(atmo_params, 0, PipelineStage::FragmentBit);

    builder.add_color_output(atmo);

    builder.map_update(atmo_params);

    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        self->resources().mapped_buffer(atmo_params)[0] = params;

        auto render_pass = recorder.bind_framebuffer(self->framebuffer());
        const auto* material = device_resources(recorder.device())[DeviceResources::AtmosphereMaterialTemplate];
        render_pass.bind_material(material, {self->descriptor_sets()[0]});
        render_pass.draw_array(3);
    });


    AtmospherePass pass;
    pass.lit = atmo;
    return pass;
}

}

