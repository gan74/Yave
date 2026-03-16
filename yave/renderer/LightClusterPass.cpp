/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

#include "LightClusterPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/shaders/ComputeProgram.h>

#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/DirectionalLightComponent.h>

#include <y/utils/memory.h>

#include <y/core/ScratchPad.h>

namespace yave {

static void fill_directional_light_buffer(shader::DirectionalLight* directionals, const SceneVisibilitySubPass& visibility, const ShadowMapPass& shadow_pass) {
    y_profile();

    usize count = 0;
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
    }
}

static void fill_point_light_buffer(shader::PointLight* points, const SceneVisibilitySubPass& visibility) {
    y_profile();

    const Scene* scene = visibility.scene_view.scene();

    usize count = 0;
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
    }
}

static void fill_spot_light_buffer(shader::SpotLight* spots, const SceneVisibilitySubPass& visibility, const ShadowMapPass& shadow_pass) {
    y_profile();

    const Scene* scene = visibility.scene_view.scene();

    usize count = 0;
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
        };
    }
}


LightClusterPass LightClusterPass::create(FrameGraph& framegraph, const GBufferPass& gbuffer, const ShadowMapSettings& shadow_settings) {
    const SceneVisibilitySubPass visibility = gbuffer.scene_pass.visibility;

    auto shadow_pass = ShadowMapPass::create(framegraph, gbuffer.scene_pass.visibility, shadow_settings);

    FrameGraphComputePassBuilder builder = framegraph.add_compute_pass("Light cluster pass");

    const auto& program = device_resources()[DeviceResources::LightClusterProgram];
    const u32 tile_size = program.local_size().x();
    y_debug_assert(program.local_size() == math::Vec3ui(tile_size, tile_size, 1));

    const math::Vec2ui size = framegraph.image_size(gbuffer.depth);
    const math::Vec2ui grid_size = math::Vec2ui(align_up_to(size.x(), tile_size), align_up_to(size.y(), tile_size)) / tile_size;

    const u32 directional_count = u32(visibility.visible->directional_lights.size());
    const u32 point_count = u32(visibility.visible->point_lights.size());
    const u32 spot_count = u32(visibility.visible->spot_lights.size());

    const auto directional_buffer = builder.declare_typed_buffer<shader::DirectionalLight>(directional_count);
    const auto point_buffer = builder.declare_typed_buffer<shader::PointLight>(point_count);
    const auto spot_buffer = builder.declare_typed_buffer<shader::SpotLight>(spot_count);

    const auto info_buffer = builder.declare_typed_buffer<shader::LightClusterInfo>();
    const auto tile_buffer = builder.declare_typed_buffer<shader::LightTile>(grid_size.x() * grid_size.y());

    builder.add_uniform_input(gbuffer.depth);
    builder.add_uniform_input(shadow_pass.shadow_map, SamplerType::Shadow);
    builder.add_uniform_input(gbuffer.scene_pass.camera);
    builder.add_storage_input(point_buffer);
    builder.add_storage_input(spot_buffer);
    builder.add_storage_input(shadow_pass.shadow_infos);
    builder.add_uniform_input(info_buffer);

    builder.add_storage_output(tile_buffer);

    builder.map_buffer(info_buffer, shader::LightClusterInfo{directional_count, point_count, spot_count, u32(0)});
    builder.map_buffer(directional_buffer);
    builder.map_buffer(point_buffer);
    builder.map_buffer(spot_buffer);

    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        auto directionals = self->resources().map_buffer(directional_buffer);
        auto points = self->resources().map_buffer(point_buffer);
        auto spots = self->resources().map_buffer(spot_buffer);

        fill_directional_light_buffer(directionals.data(), visibility, shadow_pass);
        fill_point_light_buffer(points.data(), visibility);
        fill_spot_light_buffer(spots.data(), visibility, shadow_pass);

        const auto& program = device_resources()[DeviceResources::LightClusterProgram];
        recorder.dispatch_threads(program, size, self->descriptor_set());
    });



    LightClusterPass pass;
    {
        pass.shadow_pass = shadow_pass;
        pass.grid_size = grid_size;
        pass.directionals = directional_buffer;
        pass.points = point_buffer;
        pass.spots = spot_buffer;
        pass.cluster_info = info_buffer;
        pass.tiles = tile_buffer;
    }

    return pass;
}

}

