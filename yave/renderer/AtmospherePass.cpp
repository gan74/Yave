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

#include <y/core/Chrono.h>

namespace yave {

struct AtmosphereData {
    math::Vec3 center;
    float planet_radius;

    math::Vec3 rayleigh;
    float atmosphere_height;

    math::Vec3 light_dir;
    float radius;

    math::Vec3 light_color;
    float density_falloff;
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

static FrameGraphImageId integrate_atmosphere(FrameGraph& framegraph, const AtmosphereComponent* atmosphere) {
    const math::Vec2ui size = math::Vec2ui(64, 64);
    const ImageFormat format = VK_FORMAT_R16_SFLOAT;

    const math::Vec4 atmosphere_data = {
        atmosphere->density_falloff,
        atmosphere->planet_radius,
        atmosphere->atmosphere_height,
        atmosphere->planet_radius + atmosphere->atmosphere_height,
    };

    FrameGraphPassBuilder builder = framegraph.add_pass("Atmosphere integration pass");

    const auto integrated = builder.declare_image(format, size);

    builder.add_inline_input(atmosphere_data, 0);
    builder.add_color_output(integrated);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        auto render_pass = recorder.bind_framebuffer(self->framebuffer());
        const auto* material = device_resources()[DeviceResources::AtmosphereIntegrationMaterialTemplate];
        render_pass.bind_material(material, {self->descriptor_sets()[0]});
        render_pass.draw_array(3);
    });

    return integrated;
}

AtmospherePass AtmospherePass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId lit) {
    const AtmosphereComponent* atmosphere = find_atmosphere_component(gbuffer.scene_pass.scene_view);
    const DirectionalLightComponent* sun = find_sun(gbuffer.scene_pass.scene_view);

    if(!atmosphere || !sun) {
        AtmospherePass pass;
        pass.lit = lit;
        return pass;
    }

    const auto region = framegraph.region("Atmosphere");

    auto rayleigh = [](math::Vec3 v, float scattering_strength) {
        for(auto& x : v) {
            x = std::pow(scattering_strength / x, 4.0f);
        }
        return v;
    };

    const AtmosphereData atmosphere_data {
        math::Vec3(0.0f, 0.0f, -atmosphere->planet_radius),
        atmosphere->planet_radius,

        rayleigh(atmosphere->wavelengths, atmosphere->scattering_strength),
        atmosphere->atmosphere_height,


        -sun->direction().normalized(),
        atmosphere->planet_radius + atmosphere->atmosphere_height,

        sun->color() * sun->intensity(),
        atmosphere->density_falloff,
    };


    const FrameGraphImageId integrated = integrate_atmosphere(framegraph, atmosphere);

    FrameGraphPassBuilder builder = framegraph.add_pass("Atmosphere pass");

    const auto atmo = builder.declare_copy(lit);

    builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::FragmentBit);
    builder.add_uniform_input(integrated, SamplerType::LinearClamp, 0, PipelineStage::FragmentBit);
    builder.add_uniform_input(gbuffer.scene_pass.camera_buffer, 0, PipelineStage::FragmentBit);
    builder.add_inline_input(atmosphere_data, 0);
    builder.add_color_output(atmo);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        auto render_pass = recorder.bind_framebuffer(self->framebuffer());
        const auto* material = device_resources()[DeviceResources::AtmosphereMaterialTemplate];
        render_pass.bind_material(material, {self->descriptor_sets()[0]});
        render_pass.draw_array(3);
    });


    AtmospherePass pass;
    pass.lit = atmo;
    return pass;
}

}

