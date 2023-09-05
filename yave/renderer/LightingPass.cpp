/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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


static std::tuple<const IBLProbe*, float, bool>  find_probe(const ecs::EntityWorld& world) {
    const std::array tags = {ecs::tags::not_hidden};
    for(auto id_comp : world.query<SkyLightComponent>(tags)) {
        const SkyLightComponent& sky = id_comp.component<SkyLightComponent>();
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

    const SceneView& scene = gbuffer.scene_pass.scene_view;
    auto [ibl_probe, intensity, sky] = find_probe(scene.world());
    const Texture& white = *device_resources()[DeviceResources::WhiteTexture];

    FrameGraphPassBuilder builder = framegraph.add_pass("Ambient/Sun pass");

    const bool display_sky = sky;
    const float ibl_intensity = intensity;

    const auto lit = builder.declare_copy(gbuffer.emissive);

    const auto directional_buffer = builder.declare_typed_buffer<uniform::DirectionalLight>(max_directional_lights);
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
    builder.add_storage_input(shadow_pass.shadow_params);
    builder.add_uniform_input(params_buffer);
    builder.add_color_output(lit);
    builder.map_buffer(directional_buffer);
    builder.map_buffer(params_buffer);
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        u32 count = 0;
        auto mapping = self->resources().map_buffer(directional_buffer);

        const std::array tags = {ecs::tags::not_hidden};
        for(auto&& [id, components] : scene.world().query<DirectionalLightComponent>(tags)) {
            const auto& [l] = components;

            auto shadow_indices = math::Vec4ui(u32(-1));
            if(l.cast_shadow()) {
                if(const auto it = shadow_pass.shadow_indices->find(id.as_u64()); it != shadow_pass.shadow_indices->end()) {
                    shadow_indices = it->second;
                }
            }

            mapping[count++] = {
                -l.direction().normalized(),
                std::cos(l.disk_size()),
                l.color() * l.intensity(),
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







static u32 fill_point_light_buffer(uniform::PointLight* points, const SceneView& scene) {
    y_profile();

    Y_TODO(Use octree)
    const Frustum frustum = scene.camera().frustum();

    u32 count = 0;

    const std::array tags = {ecs::tags::not_hidden};
    for(auto point : scene.world().query<TransformableComponent, PointLightComponent>(tags)) {
        const auto& [t, l] = point.components;

        const float scaled_range = l.range() * t.transform().scale().max_component();
        if(!frustum.is_inside(t.position(), scaled_range)) {
            continue;
        }

        points[count++] = {
            t.position(),
            scaled_range,

            l.color() * l.intensity(),
            std::max(math::epsilon<float>, l.falloff()),

            {},
            l.min_radius(),
        };

        if(count == max_point_lights) {
            log_msg("Too many point lights, discarding...", Log::Warning);
            break;
        }
    }
    return count;
}

template<bool Transforms>
static u32 fill_spot_light_buffer(
        uniform::SpotLight* spots,
        math::Transform<>* transforms,
        const SceneView& scene, bool render_shadows,
        const ShadowMapPass& shadow_pass) {

    y_profile();

    y_debug_assert(Transforms == !!transforms);

    Y_TODO(Use octree)
    const Frustum frustum = scene.camera().frustum();

    u32 count = 0;

    const std::array tags = {ecs::tags::not_hidden};
    for(auto&& [id, comp] : scene.world().query<TransformableComponent, SpotLightComponent>(tags)) {
        const auto& [t, l] = comp;

        const math::Vec3 forward = t.forward().normalized();
        const float scale = t.transform().scale().max_component();
        const float scaled_range = l.range() * scale;

        auto enclosing_sphere = l.enclosing_sphere();
        {
            enclosing_sphere.dist_to_center *= scale;
            enclosing_sphere.radius *= scale;
        }

        const math::Vec3 encl_sphere_center =  t.position() + forward * enclosing_sphere.dist_to_center;
        if(!frustum.is_inside(encl_sphere_center, enclosing_sphere.radius)) {
            continue;
        }

        auto shadow_indices = math::Vec4ui(u32(-1));
        if(l.cast_shadow() && render_shadows) {
            if(const auto it = shadow_pass.shadow_indices->find(id.as_u64()); it != shadow_pass.shadow_indices->end()) {
                shadow_indices = it->second;
            }
        }

        if constexpr(Transforms) {
            const float geom_radius = scaled_range * 1.1f;
            const float two_tan_angle = std::tan(l.half_angle()) * 2.0f;
            transforms[count] = t.transform().non_uniformly_scaled(math::Vec3(two_tan_angle, 1.0f, two_tan_angle) * geom_radius);
        }

        spots[count++] = {
            t.position(),
            scaled_range,

            l.color() * l.intensity(),
            std::max(math::epsilon<float>, l.falloff()),

            forward,
            l.min_radius(),

            l.attenuation_scale_offset(),
            std::sin(l.half_angle()),
            shadow_indices[0],

            encl_sphere_center,
            enclosing_sphere.radius,
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
                              const ShadowMapPass& shadow_pass) {

    const bool render_shadows = true;

    const math::Vec2ui size = framegraph.image_size(lit);
    const SceneView& scene = gbuffer.scene_pass.scene_view;

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Lighting pass");

    const auto point_buffer = builder.declare_typed_buffer<uniform::PointLight>(max_point_lights);
    const auto spot_buffer = builder.declare_typed_buffer<uniform::SpotLight>(max_spot_lights);

    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(gbuffer.color);
    builder.add_uniform_input(gbuffer.normal);
    builder.add_uniform_input(shadow_pass.shadow_map, SamplerType::Shadow);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_storage_input(point_buffer);
    builder.add_storage_input(spot_buffer);
    builder.add_storage_input(shadow_pass.shadow_params);
    builder.add_storage_output(lit);
    builder.map_buffer(point_buffer);
    builder.map_buffer(spot_buffer);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        auto points = self->resources().map_buffer(point_buffer);
        auto spots = self->resources().map_buffer(spot_buffer);

        const u32 point_count = fill_point_light_buffer(points.data(), scene);
        const u32 spot_count = fill_spot_light_buffer<false>(spots.data(), nullptr, scene, render_shadows, shadow_pass);

        if(point_count || spot_count) {
            const auto& program = device_resources()[DeviceResources::DeferredLocalsProgram];

            const math::Vec2ui light_count(point_count, spot_count);
            const auto light_count_set = DescriptorSet(InlineDescriptor(light_count));
            const std::array<DescriptorSetBase, 2> descriptor_sets = {self->descriptor_sets()[0], light_count_set};
            recorder.dispatch_size(program, size, descriptor_sets);
        }
    });
}

static void local_lights_pass(FrameGraph& framegraph,
                              FrameGraphMutableImageId lit,
                              const GBufferPass& gbuffer,
                              const ShadowMapPass& shadow_pass) {

    const bool render_shadows = true;
    const SceneView& scene = gbuffer.scene_pass.scene_view;

    FrameGraphMutableImageId copied_depth;

    {
        FrameGraphPassBuilder builder = framegraph.add_pass("Point light pass");

        const auto point_buffer = builder.declare_typed_buffer<uniform::PointLight>(max_point_lights);

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
            const u32 point_count = fill_point_light_buffer(points.data(), scene);

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

        const auto spot_buffer = builder.declare_typed_buffer<uniform::SpotLight>(max_spot_lights);
        const auto transform_buffer = builder.declare_typed_buffer<math::Transform<>>(max_spot_lights);

        builder.add_uniform_input(gbuffer.scene_pass.camera, PipelineStage::VertexBit);
        builder.add_storage_input(spot_buffer, PipelineStage::VertexBit);
        builder.add_uniform_input(gbuffer.depth);
        builder.add_uniform_input(gbuffer.color);
        builder.add_uniform_input(gbuffer.normal);
        builder.add_uniform_input(shadow_pass.shadow_map, SamplerType::Shadow);
        builder.add_storage_input(shadow_pass.shadow_params);
        builder.add_attrib_input(transform_buffer);
        builder.add_depth_output(copied_depth);
        builder.add_color_output(lit);
        builder.map_buffer(spot_buffer);
        builder.map_buffer(transform_buffer);
        builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
            auto spots = self->resources().map_buffer(spot_buffer);
            auto transforms = self->resources().map_buffer(transform_buffer);

            const u32 spot_count = fill_spot_light_buffer<true>(spots.data(), transforms.data(), scene, render_shadows, shadow_pass);

            if(!spot_count) {
                return;
            }

            const auto* material = device_resources()[DeviceResources::DeferredSpotLightMaterialTemplate];
            render_pass.bind_material_template(material, self->descriptor_sets());

            {
                render_pass.bind_per_instance_attrib_buffers(self->resources().buffer<BufferUsage::AttributeBit>(transform_buffer));

                const StaticMesh& cone = *device_resources()[DeviceResources::ConeMesh];
                render_pass.draw(cone.draw_data(), spot_count);
            }
        });
    }
}

LightingPass LightingPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId ao, const LightingSettings& settings) {
    const auto region = framegraph.region("Lighting");

    const SceneView& scene = gbuffer.scene_pass.scene_view;

    LightingPass pass;
    pass.shadow_pass = ShadowMapPass::create(framegraph, scene, settings.shadow_settings);

    const auto lit = ambient_pass(framegraph, gbuffer, pass.shadow_pass, ao);

    if(settings.use_compute_for_locals) {
        local_lights_pass_compute(framegraph, lit, gbuffer, pass.shadow_pass);
    } else {
        local_lights_pass(framegraph, lit, gbuffer, pass.shadow_pass);
    }

    pass.lit = lit;
    return pass;
}

}

