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

#include "LightingPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/scene/Scene.h>

#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/ecs/EntityWorld.h>

#include <y/core/ScratchPad.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

static constexpr usize max_directional_lights = 16;
static constexpr usize max_point_lights = 1024;
static constexpr usize max_spot_lights = 1024;


static u32 fill_directional_light_buffer(shader::DirectionalLight* directionals, const SceneVisibilitySubPass& visibility, const ShadowMapPass& shadow_pass) {
    y_profile();

    u32 count = 0;
    for(const DirectionalLightObject* obj : visibility.visible->directional_lights) {
        const auto& light = obj->component;

        auto shadow_indices = math::Vec4ui(u32(-1));
        if(light.cast_shadow()) {
            if(const auto it = shadow_pass.shadow_indices->find(&light); it != shadow_pass.shadow_indices->end()) {
                shadow_indices = it->second;
            }
        }

        directionals[count++] = {
            -light.direction().normalized(),
            std::cos(light.disk_size()),
            light.color() * light.intensity(),
            u32(light.cast_shadow() ? 1 : 0),
            shadow_indices
        };

        if(count == max_directional_lights) {
            log_msg("Too many directional lights, discarding...", Log::Warning);
            break;
        }
    }
    return count;
}

static u32 fill_point_light_buffer(shader::PointLight* points, const SceneVisibilitySubPass& visibility) {
    y_profile();

    const Scene* scene = visibility.scene_view.scene();

    u32 count = 0;
    for(const PointLightObject* obj : visibility.visible->point_lights) {
        const math::Transform<> transform = scene->transform(*obj);
        const auto& light = obj->component;

        const float scale = transform.scale().max_component();
        const float scaled_range = light.range() * scale;

        points[count++] = {
            transform.position(),
            scaled_range,

            light.color() * light.intensity(),
            std::max(math::epsilon<float>, light.falloff()),

            {},
            light.min_radius(),
        };

        if(count == max_point_lights) {
            log_msg("Too many point lights, discarding...", Log::Warning);
            break;
        }
    }
    return count;
}

template<bool SetTransform>
static u32 fill_spot_light_buffer(shader::SpotLight* spots, const SceneVisibilitySubPass& visibility, const ShadowMapPass& shadow_pass) {
    y_profile();

    const Scene* scene = visibility.scene_view.scene();

    u32 count = 0;
    for(const SpotLightObject* obj : visibility.visible->spot_lights) {
        const math::Transform<> transform = scene->transform(*obj);
        const auto& light = obj->component;

        const math::Vec3 forward = transform.forward().normalized();
        const float scale = transform.scale().max_component();
        const float scaled_range = light.range() * scale;

        auto enclosing_sphere = light.enclosing_sphere();
        {
            enclosing_sphere.dist_to_center *= scale;
            enclosing_sphere.radius *= scale;
        }

        const math::Vec3 encl_sphere_center =  transform.position() + forward * enclosing_sphere.dist_to_center;

        auto shadow_indices = math::Vec4ui(u32(-1));
        if(light.cast_shadow()) {
            if(const auto it = shadow_pass.shadow_indices->find(&light); it != shadow_pass.shadow_indices->end()) {
                shadow_indices = it->second;
            }
        }

        math::Transform<> draw_model;
        if constexpr(SetTransform) {
            const float geom_radius = scaled_range * 1.1f;
            const float two_tan_angle = std::tan(light.half_angle()) * 2.0f;
            draw_model = transform.non_uniformly_scaled(math::Vec3(two_tan_angle, 1.0f, two_tan_angle) * geom_radius);
        }

        spots[count++] = {
            transform.position(),
            scaled_range,

            light.color() * light.intensity(),
            std::max(math::epsilon<float>, light.falloff()),

            forward,
            light.min_radius(),

            light.attenuation_scale_offset(),
            std::sin(light.half_angle()),
            shadow_indices[0],

            encl_sphere_center,
            enclosing_sphere.radius,

            draw_model,
        };

        if(count == max_spot_lights) {
            log_msg("Too many spot lights, discarding...", Log::Warning);
            break;
        }
    }

    return count;
}


LightingPass LightingPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, const LightingSettings& settings) {
    LightingPass pass;
    pass.shadow_pass = ShadowMapPass::create(framegraph, gbuffer.scene_pass.visibility, settings.shadow_settings);

    {
        const SceneVisibilitySubPass visibility = gbuffer.scene_pass.visibility;

        FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Lighting pass");

        const auto lit = builder.declare_copy(gbuffer.emissive);
        const math::Vec2ui size = framegraph.image_size(lit);

        const auto directional_buffer = builder.declare_typed_buffer<shader::DirectionalLight>(max_directional_lights);
        const auto point_buffer = builder.declare_typed_buffer<shader::PointLight>(max_point_lights);
        const auto spot_buffer = builder.declare_typed_buffer<shader::SpotLight>(max_spot_lights);

        builder.add_uniform_input(gbuffer.depth);
        builder.add_uniform_input(gbuffer.color);
        builder.add_uniform_input(gbuffer.normal);
        builder.add_uniform_input(pass.shadow_pass.shadow_map, SamplerType::Shadow);
        builder.add_uniform_input(gbuffer.scene_pass.camera);
        builder.add_storage_input(directional_buffer);
        builder.add_storage_input(point_buffer);
        builder.add_storage_input(spot_buffer);
        builder.add_storage_input(pass.shadow_pass.shadow_infos);

        builder.add_storage_output(lit);

        builder.map_buffer(directional_buffer);
        builder.map_buffer(point_buffer);
        builder.map_buffer(spot_buffer);

        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            auto directionals = self->resources().map_buffer(directional_buffer);
            auto points = self->resources().map_buffer(point_buffer);
            auto spots = self->resources().map_buffer(spot_buffer);

            const u32 directional_count = fill_directional_light_buffer(directionals.data(), visibility, pass.shadow_pass);
            const u32 point_count = fill_point_light_buffer(points.data(), visibility);
            const u32 spot_count = fill_spot_light_buffer<false>(spots.data(), visibility, pass.shadow_pass);
            const math::Vec4ui light_count(directional_count, point_count, spot_count, 0);

            const auto& program = device_resources()[settings.debug_tiles ? DeviceResources::DeferredSingleDebugPassProgram : DeviceResources::DeferredSinglePassProgram];

            core::ScratchVector<Descriptor> descs(self->descriptor_set().descriptors().size() + 1);
            descs.push_back(self->descriptor_set().descriptors().begin(), self->descriptor_set().descriptors().end());
            descs.emplace_back(InlineDescriptor(light_count));
            recorder.dispatch_threads(program, size, DescriptorSetProxy(descs));
        });

        pass.lit = lit;
    }

    return pass;
}

}

