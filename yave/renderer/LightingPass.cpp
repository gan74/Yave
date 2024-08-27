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

#include "LightingPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/scene/Scene.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/graphics/images/IBLProbe.h>

#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/SkyLightComponent.h>
#include <yave/ecs/EntityWorld.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <bit>


namespace yave {

static constexpr usize max_directional_lights = 16;
static constexpr usize max_point_lights = 1024;
static constexpr usize max_spot_lights = 1024;


static std::tuple<const IBLProbe*, float, bool> find_probe(const SceneView& scene_view) {
    for(const SkyLightObject& obj : scene_view.scene()->sky_lights()) {
        if((obj.visibility_mask & scene_view.visibility_mask()) == 0) {
            continue;
        }

        const SkyLightComponent& sky = obj.component;
        if(const IBLProbe* probe = sky.probe().get()) {
            y_debug_assert(!probe->is_null());
            return {probe, sky.intensity(), sky.display_sky()};
        }
    }

    return {device_resources().empty_probe().get(), 1.0f, true};
}


static FrameGraphMutableImageId ambient_pass(FrameGraph& framegraph,
                                             const GBufferPass& gbuffer,
                                             const ShadowMapPass& shadow_pass,
                                             FrameGraphImageId ao) {

    const SceneView& scene_view = gbuffer.scene_pass.scene_view;
    const SceneVisibility& visibility = *gbuffer.scene_pass.visibility.visible;

    auto [ibl_probe, intensity, sky] = find_probe(scene_view);
    const Texture& white = *device_resources()[DeviceResources::WhiteTexture];

    FrameGraphPassBuilder builder = framegraph.add_pass("Ambient/Sun pass");

    const bool display_sky = sky;
    const float ibl_intensity = intensity;

    const auto lit = builder.declare_copy(gbuffer.emissive);

    const auto directional_buffer = builder.declare_typed_buffer<shader::DirectionalLight>(max_directional_lights);
    const auto params_buffer = builder.declare_typed_buffer<math::Vec4ui>();

    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(gbuffer.color);
    builder.add_uniform_input(gbuffer.normal);
    builder.add_uniform_input(shadow_pass.shadow_map, SamplerType::Shadow);
    builder.add_uniform_input_with_default(ao, Descriptor(white));
    builder.add_external_input(*ibl_probe);
    builder.add_external_input(Descriptor(device_resources().brdf_lut(), SamplerType::LinearClamp));
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_storage_input(directional_buffer);
    builder.add_storage_input(shadow_pass.shadow_infos);
    builder.add_uniform_input(params_buffer);
    builder.add_color_output(lit);
    builder.map_buffer(directional_buffer);
    builder.map_buffer(params_buffer);
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        u32 count = 0;
        auto mapping = self->resources().map_buffer(directional_buffer);

        for(const DirectionalLightObject* obj : visibility.directional_lights) {
            const auto& [light, id, _] = *obj;

            auto shadow_indices = math::Vec4ui(u32(-1));
            if(light.cast_shadow()) {
                if(const auto it = shadow_pass.shadow_indices->find(&light); it != shadow_pass.shadow_indices->end()) {
                    shadow_indices = it->second;
                }
            }

            mapping[count++] = {
                -light.direction().normalized(),
                std::cos(light.disk_size()),
                light.color() * light.intensity(),
                0,
                shadow_indices
            };

            if(count == mapping.size()) {
                log_msg("Too many directional lights, discarding...", Log::Warning);
                break;
            }
        }

        {
            auto params = self->resources().map_buffer(params_buffer);
            params[0] = {
                count,
                display_sky ? 1 : 0,
                std::bit_cast<u32>(ibl_intensity),
                0
            };
        }

        const auto* material = device_resources()[DeviceResources::DeferredAmbientMaterialTemplate];
        render_pass.bind_material_template(material, self->descriptor_sets());
        render_pass.draw_array(3);
    });

    return lit;
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

static void local_lights_pass_compute(FrameGraph& framegraph,
                              FrameGraphMutableImageId lit,
                              const GBufferPass& gbuffer,
                              const ShadowMapPass& shadow_pass,
                              bool debug_tiles = false) {

    const math::Vec2ui size = framegraph.image_size(lit);
    const SceneVisibilitySubPass visibility = gbuffer.scene_pass.visibility;

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Lighting pass");

    const auto point_buffer = builder.declare_typed_buffer<shader::PointLight>(max_point_lights);
    const auto spot_buffer = builder.declare_typed_buffer<shader::SpotLight>(max_spot_lights);

    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(gbuffer.color);
    builder.add_uniform_input(gbuffer.normal);
    builder.add_uniform_input(shadow_pass.shadow_map, SamplerType::Shadow);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_storage_input(point_buffer);
    builder.add_storage_input(spot_buffer);
    builder.add_storage_input(shadow_pass.shadow_infos);
    builder.add_storage_output(lit);
    builder.map_buffer(point_buffer);
    builder.map_buffer(spot_buffer);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        auto points = self->resources().map_buffer(point_buffer);
        auto spots = self->resources().map_buffer(spot_buffer);

        const u32 point_count = fill_point_light_buffer(points.data(), visibility);
        const u32 spot_count = fill_spot_light_buffer<false>(spots.data(), visibility, shadow_pass);

        if(point_count || spot_count) {
            const auto& program = device_resources()[debug_tiles ? DeviceResources::DeferredLocalsDebugProgram : DeviceResources::DeferredLocalsProgram];

            const math::Vec2ui light_count(point_count, spot_count);
            const auto light_count_set = DescriptorSet(InlineDescriptor(light_count));
            const std::array<DescriptorSetBase, 2> descriptor_sets = {self->descriptor_sets()[0], light_count_set};
            recorder.dispatch_threads(program, size, descriptor_sets);
        }
    });
}

static void local_lights_pass(FrameGraph& framegraph,
                              FrameGraphMutableImageId lit,
                              const GBufferPass& gbuffer,
                              const ShadowMapPass& shadow_pass) {

    const SceneVisibilitySubPass visibility = gbuffer.scene_pass.visibility;

    FrameGraphMutableImageId copied_depth;

    {
        FrameGraphPassBuilder builder = framegraph.add_pass("Point light pass");

        const auto point_buffer = builder.declare_typed_buffer<shader::PointLight>(max_point_lights);

        // Moving this down causes a reused resource assert
        copied_depth = builder.declare_copy(gbuffer.depth); // extra copy for nothing =(

        builder.add_uniform_input(gbuffer.scene_pass.camera, PipelineStage::VertexBit);
        builder.add_storage_input(point_buffer, PipelineStage::VertexBit);
        builder.add_uniform_input(gbuffer.depth);
        builder.add_uniform_input(gbuffer.color);
        builder.add_uniform_input(gbuffer.normal);
        builder.add_depth_output(copied_depth);
        builder.add_color_output(lit);
        builder.map_buffer(point_buffer);
        builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
            auto points = self->resources().map_buffer(point_buffer);
            const u32 point_count = fill_point_light_buffer(points.data(), visibility);

            if(!point_count) {
                return;
            }

            const auto* material = device_resources()[DeviceResources::DeferredPointLightMaterialTemplate];
            render_pass.bind_material_template(material, self->descriptor_sets());
            {
                const StaticMesh& sphere = *device_resources()[DeviceResources::SimpleSphereMesh];
                render_pass.draw(sphere.draw_data(), point_count);
            }
        });
    }

    {
        FrameGraphPassBuilder builder = framegraph.add_pass("Spot light pass");

        const auto spot_buffer = builder.declare_typed_buffer<shader::SpotLight>(max_spot_lights);

        builder.add_uniform_input(gbuffer.scene_pass.camera);
        builder.add_storage_input(spot_buffer);
        builder.add_uniform_input(gbuffer.depth);
        builder.add_uniform_input(gbuffer.color);
        builder.add_uniform_input(gbuffer.normal);
        builder.add_uniform_input(shadow_pass.shadow_map, SamplerType::Shadow);
        builder.add_storage_input(shadow_pass.shadow_infos);
        builder.add_depth_output(copied_depth);
        builder.add_color_output(lit);
        builder.map_buffer(spot_buffer);
        builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
            auto spots = self->resources().map_buffer(spot_buffer);
            const u32 spot_count = fill_spot_light_buffer<true>(spots.data(), visibility, shadow_pass);

            if(!spot_count) {
                return;
            }

            const auto* material = device_resources()[DeviceResources::DeferredSpotLightMaterialTemplate];
            render_pass.bind_material_template(material, self->descriptor_sets());

            const StaticMesh& cone = *device_resources()[DeviceResources::ConeMesh];
            render_pass.draw(cone.draw_data(), spot_count);
        });
    }
}

LightingPass LightingPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId ao, const LightingSettings& settings) {
    const auto region = framegraph.region("Lighting");

    LightingPass pass;
    pass.shadow_pass = ShadowMapPass::create(framegraph, gbuffer.scene_pass.visibility, settings.shadow_settings);

    const auto lit = ambient_pass(framegraph, gbuffer, pass.shadow_pass, ao);

    if(settings.use_compute_for_locals) {
        local_lights_pass_compute(framegraph, lit, gbuffer, pass.shadow_pass, settings.debug_tiles);
    } else {
        local_lights_pass(framegraph, lit, gbuffer, pass.shadow_pass);
    }

    pass.lit = lit;
    return pass;
}

}

