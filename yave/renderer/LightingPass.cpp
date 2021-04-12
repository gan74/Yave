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


namespace yave {

static constexpr ImageFormat lighting_format = VK_FORMAT_R16G16B16A16_SFLOAT;
static constexpr usize max_directional_lights = 16;
static constexpr usize max_point_lights = 1024;
static constexpr usize max_spot_lights = 1024;
static constexpr usize max_shadow_lights = 128;


static const IBLProbe* find_probe(const ecs::EntityWorld& world) {
    for(const SkyLightComponent& sky : world.components<SkyLightComponent>()) {
        if(const IBLProbe* probe = sky.probe().get()) {
            y_debug_assert(!probe->is_null());
            return probe;
        }
    }

    return device_resources().empty_probe().get();
}


static FrameGraphMutableImageId ambient_pass(FrameGraph& framegraph,
                                             const GBufferPass& gbuffer,
                                             FrameGraphImageId ao) {

    const SceneView& scene = gbuffer.scene_pass.scene_view;
    const IBLProbe* ibl_probe = find_probe(scene.world());
    const Texture& white = *device_resources()[DeviceResources::WhiteTexture];

    FrameGraphPassBuilder builder = framegraph.add_pass("Ambient/Sun pass");

    const auto lit = builder.declare_copy(gbuffer.emissive);

    const auto directional_buffer = builder.declare_typed_buffer<uniform::DirectionalLight>(max_directional_lights);
    const auto light_count_buffer = builder.declare_typed_buffer<u32>(1);

    builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::FragmentBit);
    builder.add_uniform_input(gbuffer.color, 0, PipelineStage::FragmentBit);
    builder.add_uniform_input(gbuffer.normal, 0, PipelineStage::FragmentBit);
    builder.add_uniform_input_with_default(ao, Descriptor(white), 0, PipelineStage::FragmentBit);
    builder.add_external_input(*ibl_probe, 0, PipelineStage::FragmentBit);
    builder.add_external_input(Descriptor(device_resources().brdf_lut(), SamplerType::LinearClamp), 0, PipelineStage::FragmentBit);
    builder.add_uniform_input(gbuffer.scene_pass.camera_buffer, 0, PipelineStage::FragmentBit);
    builder.add_storage_input(directional_buffer, 0, PipelineStage::FragmentBit);
    builder.add_uniform_input(light_count_buffer, 0, PipelineStage::FragmentBit);
    builder.add_color_output(lit);
    builder.map_update(directional_buffer);
    builder.map_update(light_count_buffer);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        u32 light_count = 0;
        TypedMapping<uniform::DirectionalLight> mapping = self->resources().mapped_buffer(directional_buffer);
        for(auto [l] : scene.world().view<DirectionalLightComponent>().components()) {
            mapping[light_count++] = {
                    -l.direction().normalized(),
                    0,
                    l.color() * l.intensity(),
                    0
                };
        }
        self->resources().mapped_buffer(light_count_buffer)[0] = light_count;

        auto render_pass = recorder.bind_framebuffer(self->framebuffer());
        const auto* material = device_resources()[DeviceResources::DeferredAmbientMaterialTemplate];
        render_pass.bind_material(material, {self->descriptor_sets()[0]});
        render_pass.draw_array(3);
    });

    return lit;
}







static u32 fill_point_light_buffer(uniform::PointLight* points, const SceneView& scene) {
    const Frustum frustum = scene.camera().frustum();

    u32 count = 0;
    for(auto [t, l] : scene.world().view<TransformableComponent, PointLightComponent>().components()) {
        if(!frustum.is_inside(t.position(), l.radius())) {
            continue;
        }

        points[count++] = {
            t.position(),
            l.radius(),
            l.color() * l.intensity(),
            std::max(math::epsilon<float>, l.falloff())
        };
    }
    return count;
}

template<bool Transforms>
static std::pair<u32, u32> fill_spot_light_buffer(
        uniform::SpotLight* spots,
        uniform::ShadowMapParams* shadows,
        math::Transform<>* transforms,
        const SceneView& scene, bool render_shadows,
        const ShadowMapPass& shadow_pass) {

    y_debug_assert(Transforms == !!transforms);

    const Frustum frustum = scene.camera().frustum();

    u32 count = 0;
    u32 shadow_count = 0;
    for(auto spot : scene.world().view<TransformableComponent, SpotLightComponent>()) {
        auto [t, l] = spot.components();

        const auto enclosing_sphere = l.enclosing_sphere();
        const math::Vec3 encl_sphere_center = t.forward() * enclosing_sphere.dist_to_center + t.position();
        if(!frustum.is_inside(encl_sphere_center, enclosing_sphere.radius)) {
            continue;
        }

        u32 shadow_index = u32(-1);
        if(l.cast_shadow() && render_shadows) {
            if(const auto it = shadow_pass.sub_passes->lights.find(spot.id().as_u64()); it != shadow_pass.sub_passes->lights.end()) {
                shadows[shadow_index = shadow_count++] = it->second;
            }
        }

        if constexpr(Transforms) {
            const float geom_radius = l.radius() * 1.1f;
            const float two_tan_angle = std::tan(l.half_angle()) * 2.0f;
            transforms[count] = t.transform().non_uniformly_scaled(math::Vec3(two_tan_angle, 1.0f, two_tan_angle) * geom_radius);
        }

        spots[count++] = {
            t.position(),
            l.radius(),
            l.color() * l.intensity(),
            std::max(math::epsilon<float>, l.falloff()),
            t.forward(),
            std::cos(l.half_angle()),
            encl_sphere_center,
            enclosing_sphere.radius,
            std::max(math::epsilon<float>, l.angle_exponent()),
            shadow_index,
            {}
        };
    }

    return {count, shadow_count};
}

static void local_lights_pass_compute(FrameGraph& framegraph,
                              FrameGraphMutableImageId lit,
                              const GBufferPass& gbuffer,
                              const ShadowMapPass& shadow_pass) {

    const bool render_shadows = true;

    const math::Vec2ui size = framegraph.image_size(lit);
    const SceneView& scene = gbuffer.scene_pass.scene_view;

    FrameGraphPassBuilder builder = framegraph.add_pass("Lighting pass");

    const auto point_buffer = builder.declare_typed_buffer<uniform::PointLight>(max_point_lights);
    const auto spot_buffer = builder.declare_typed_buffer<uniform::SpotLight>(max_spot_lights);
    const auto shadow_buffer = builder.declare_typed_buffer<uniform::ShadowMapParams>(max_shadow_lights);

    builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::ComputeBit);
    builder.add_uniform_input(gbuffer.color, 0, PipelineStage::ComputeBit);
    builder.add_uniform_input(gbuffer.normal, 0, PipelineStage::ComputeBit);
    builder.add_uniform_input(shadow_pass.shadow_map, SamplerType::Shadow, 0, PipelineStage::ComputeBit);
    builder.add_uniform_input(gbuffer.scene_pass.camera_buffer, 0, PipelineStage::ComputeBit);
    builder.add_storage_input(point_buffer, 0, PipelineStage::ComputeBit);
    builder.add_storage_input(spot_buffer, 0, PipelineStage::ComputeBit);
    builder.add_storage_input(shadow_buffer, 0, PipelineStage::ComputeBit);
    builder.add_storage_output(lit, 0, PipelineStage::ComputeBit);
    builder.map_update(point_buffer);
    builder.map_update(spot_buffer);
    builder.map_update(shadow_buffer);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        TypedMapping<uniform::PointLight> points = self->resources().mapped_buffer(point_buffer);
        TypedMapping<uniform::SpotLight> spots = self->resources().mapped_buffer(spot_buffer);
        TypedMapping<uniform::ShadowMapParams> shadows = self->resources().mapped_buffer(shadow_buffer);

        const u32 point_count = fill_point_light_buffer(points.data(), scene);
        const auto [spot_count, shadow_count] = fill_spot_light_buffer<false>(spots.data(), shadows.data(), nullptr, scene, render_shadows, shadow_pass);

        struct PushData {
            u32 point_count;
            u32 spot_count;
            u32 shadow_count;
        };

        if(point_count || spot_count) {
            const auto& program = device_resources()[DeviceResources::DeferredLocalsProgram];
            recorder.dispatch_size(program, size, {self->descriptor_sets()[0]}, PushData{point_count, spot_count, shadow_count});
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

        builder.add_uniform_input(gbuffer.scene_pass.camera_buffer, 0, PipelineStage::VertexBit);
        builder.add_storage_input(point_buffer, 0, PipelineStage::VertexBit);
        builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::FragmentBit);
        builder.add_uniform_input(gbuffer.color, 0, PipelineStage::FragmentBit);
        builder.add_uniform_input(gbuffer.normal, 0, PipelineStage::FragmentBit);
        builder.add_depth_output(copied_depth);
        builder.add_color_output(lit);
        builder.map_update(point_buffer);
        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            TypedMapping<uniform::PointLight> points = self->resources().mapped_buffer(point_buffer);
            const u32 point_count = fill_point_light_buffer(points.data(), scene);

            if(!point_count) {
                return;
            }

            auto render_pass = recorder.bind_framebuffer(self->framebuffer());
            const auto* material = device_resources()[DeviceResources::DeferredPointLightMaterialTemplate];
            render_pass.bind_material(material, {self->descriptor_sets()[0]});
            {
                const StaticMesh& sphere = *device_resources()[DeviceResources::SimpleSphereMesh];
                VkDrawIndexedIndirectCommand indirect = sphere.indirect_data();
                indirect.instanceCount = point_count;
                render_pass.bind_buffers(sphere.triangle_buffer(), sphere.vertex_buffer());
                render_pass.draw(indirect);
            }
        });
    }

    {
        FrameGraphPassBuilder builder = framegraph.add_pass("Spot light pass");

        const auto spot_buffer = builder.declare_typed_buffer<uniform::SpotLight>(max_spot_lights);
        const auto transform_buffer = builder.declare_typed_buffer<math::Transform<>>(max_spot_lights);
        const auto shadow_buffer = builder.declare_typed_buffer<uniform::ShadowMapParams>(max_shadow_lights);

        builder.add_uniform_input(gbuffer.scene_pass.camera_buffer, 0, PipelineStage::VertexBit);
        builder.add_storage_input(spot_buffer, 0, PipelineStage::VertexBit);
        builder.add_uniform_input(gbuffer.depth, 0, PipelineStage::FragmentBit);
        builder.add_uniform_input(gbuffer.color, 0, PipelineStage::FragmentBit);
        builder.add_uniform_input(gbuffer.normal, 0, PipelineStage::FragmentBit);
        builder.add_uniform_input(shadow_pass.shadow_map, SamplerType::Shadow, 0, PipelineStage::FragmentBit);
        builder.add_storage_input(shadow_buffer, 0, PipelineStage::ComputeBit);
        builder.add_attrib_input(transform_buffer);
        builder.add_depth_output(copied_depth);
        builder.add_color_output(lit);
        builder.map_update(spot_buffer);
        builder.map_update(transform_buffer);
        builder.map_update(shadow_buffer);
        builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            TypedMapping<uniform::SpotLight> spots = self->resources().mapped_buffer(spot_buffer);
            TypedMapping<uniform::ShadowMapParams> shadows = self->resources().mapped_buffer(shadow_buffer);
            TypedMapping<math::Transform<>> transforms = self->resources().mapped_buffer(transform_buffer);

            const auto [spot_count, shadow_count] = fill_spot_light_buffer<true>(spots.data(), shadows.data(), transforms.data(), scene, render_shadows, shadow_pass);

            if(!spot_count) {
                return;
            }

            auto render_pass = recorder.bind_framebuffer(self->framebuffer());
            const auto* material = device_resources()[DeviceResources::DeferredSpotLightMaterialTemplate];
            render_pass.bind_material(material, {self->descriptor_sets()[0]});

            {
                const auto transforms = self->resources().buffer<BufferUsage::AttributeBit>(transform_buffer);
                render_pass.bind_attrib_buffers({}, {transforms});

                const StaticMesh& cone = *device_resources()[DeviceResources::ConeMesh];
                VkDrawIndexedIndirectCommand indirect = cone.indirect_data();
                indirect.instanceCount = spot_count;
                render_pass.bind_buffers(cone.triangle_buffer(), cone.vertex_buffer());
                render_pass.draw(indirect);
            }
        });
    }
}


LightingPass LightingPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, FrameGraphImageId ao, const LightingSettings& settings) {
    const auto region = framegraph.region("Lighting");

    const SceneView& scene = gbuffer.scene_pass.scene_view;

    LightingPass pass;
    pass.shadow_pass = ShadowMapPass::create(framegraph, scene, settings.shadow_settings);

    const auto lit = ambient_pass(framegraph, gbuffer, ao);

    if(settings.use_compute_for_locals) {
        local_lights_pass_compute(framegraph, lit, gbuffer, pass.shadow_pass);
    } else {
        local_lights_pass(framegraph, lit, gbuffer, pass.shadow_pass);
    }

    pass.lit = lit;
    return pass;
}

}

