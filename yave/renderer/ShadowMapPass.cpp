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

#include "ShadowMapPass.h"

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>

#include <yave/systems/OctreeSystem.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/ecs/EntityWorld.h>

#include <y/utils/log.h>

#include <limits>

namespace yave {

struct SubAtlas {
    math::Vec2ui pos;
    u32 subs = 4;
};

class SubAtlasAllocator {
    public:
        SubAtlasAllocator(u32 first_level_size) : _first_level_size(first_level_size) {
        }

        std::pair<math::Vec2ui, u32> alloc(u32 level) {
            y_always_assert(level < _levels.size(), "Invalid atlas level");
            const u32 size = _first_level_size >> level;
            const u32 index = _levels[level].subs;
            if(index != 4) {
                ++_levels[level].subs;
                return {math::Vec2ui(index >> 1, index & 1) * size + _levels[level].pos, size};
            }

            if(level == 0) {
                if(_first_level_index < u32(_levels.size())) {
                    return {math::Vec2ui(0, _first_level_index++) * _first_level_size, _first_level_size};
                }
                return {math::Vec2ui(), 0};
            }

            const math::Vec2ui pos = alloc(level - 1).first;
            _levels[level] = {pos, 0};
            return alloc(level);
        };

    private:
        u32 _first_level_size;
        u32 _first_level_index = 0;
        std::array<SubAtlas, 32> _levels;
};






struct SubPass {
    SceneRenderSubPass scene_pass;
    math::Vec2ui viewport_offset;
    u32 viewport_size;
    uniform::ShadowMapParams params;
};

static SubPass create_sub_pass(FrameGraphPassBuilder& builder,
                              math::Vec2ui offset, u32 size, // from allocator
                              const SceneView& light_view,
                              const math::Vec2& uv_mul) {
    y_profile();

    if(!size) {
        log_msg("Unable to allocate shadow altas: too many shadow casters", Log::Warning);
    }

    const float size_f = float(size);
    const uniform::ShadowMapParams params = {
        light_view.camera().view_proj_matrix(),
        math::Vec2(offset) * uv_mul,
        uv_mul * size_f,
        size_f,
        1.0f / size_f,
        0, 0,
    };

    return SubPass{
        SceneRenderSubPass::create(builder, light_view),
        offset, size,
        params
    };
}

static math::Matrix4<> flip_for_backfaces(math::Matrix4<> proj) {
    proj[0] = -proj[0];
    return proj;
}



static Camera spotlight_camera(const TransformableComponent& tr, const SpotLightComponent& light) {
    const float z_near = light.min_radius();

    Camera camera(
        math::look_at(tr.position(), tr.position() + tr.forward(), tr.up()),
        flip_for_backfaces(math::perspective(light.half_angle() * 2.0f, 1.0f, z_near))
    );
    camera.set_far(light.range() * tr.transform().scale().max_component());
    y_debug_assert(!camera.is_orthographic());
    return camera;
}

static Camera directional_camera(const Camera& cam, const DirectionalLightComponent& light, u32 size, float near_dist, float far_dist) {
    y_debug_assert(near_dist < far_dist && near_dist >= 0.0f);

    const Frustum frustum = cam.frustum();
    const math::Vec3 cam_fwd = cam.forward();
    const math::Vec3 cam_pos = cam.position();

    const auto& planes = frustum.planes();

    const math::Vec3 top_left = planes[Frustum::Left].normal.cross(planes[Frustum::Top].normal).normalized();
    const float top_left_dist = far_dist / top_left.dot(cam_fwd);
    const math::Vec3 top_left_far = cam_pos + top_left_dist * top_left;

    const math::Vec3 center = cam_pos + cam_fwd * (near_dist + (far_dist - near_dist) * 0.5f);
    const float radius = (center - top_left_far).length();

    const math::Vec3 light_dir = light.direction();
    const math::Vec3 light_side = std::abs(light_dir.x()) < std::abs(light_dir.y()) ? math::Vec3(1.0f, 0.0f, 0.0f) : math::Vec3(0.0f, 1.0f, 0.0f);
    const math::Vec3 light_up = light_dir.cross(light_side).normalized();

    const float texel_world_size = (radius * 16.0f) / size; // Why 16 ?

    const math::Matrix3<> light_view = math::look_at(math::Vec3(), light_dir, light_up).to<3, 3>();
    math::Vec3 view_center = light_view * center;
    for(float& c : view_center) {
        c = std::floor(c / texel_world_size) * texel_world_size;
    }
    const math::Vec3 snapped = light_view.inverse() * view_center;

    const float z_bound = 1000.0f;

    Camera camera;
    camera.set_view(math::look_at(snapped, snapped + light_dir, light_up));
    camera.set_proj(flip_for_backfaces(math::ortho(-radius, radius, -radius, radius, z_bound, -z_bound)));
    y_debug_assert(camera.is_orthographic());
    return camera;
}



struct ShadowCastingLights {
    core::Vector<std::tuple<ecs::EntityId, const DirectionalLightComponent*>> directionals;
    core::Vector<std::tuple<ecs::EntityId, const TransformableComponent*, const SpotLightComponent*>> spots;
};

static ShadowCastingLights collect_shadow_casting_lights(const SceneView& scene) {
    const std::array tags = {ecs::tags::not_hidden};
    const ecs::EntityWorld& world = scene.world();

    ShadowCastingLights shadow_casters;

    shadow_casters.directionals.set_min_capacity(world.component_set<DirectionalLightComponent>().size());
    for(auto&& [id, comp] : world.query<DirectionalLightComponent>(tags)) {
        const auto& [l] = comp;
        if(!l.cast_shadow()) {
            continue;
        }
        shadow_casters.directionals.push_back({id, &l});
    }

    auto collect_spots = [&](auto&& query) {
        shadow_casters.spots.set_min_capacity(query.size());
        for(auto&& [id, comp] : query) {
            const auto& [t, l] = comp;
            if(!l.cast_shadow()) {
                continue;
            }
            shadow_casters.spots.push_back({id, &t, &l});
        }
    };

    if(const OctreeSystem* octree_system = world.find_system<OctreeSystem>()) {
        const core::Vector<ecs::EntityId> visible = octree_system->find_entities(scene.camera());
        collect_spots(world.query<TransformableComponent, SpotLightComponent>(visible, tags));
    } else {
        collect_spots(world.query<TransformableComponent, SpotLightComponent>(tags));
    }

    return shadow_casters;
}

static float total_occupancy(const ShadowCastingLights& lights) {
    auto occupancy = [](u32 lod) {
        return 1.0f / (1 << lod);
    };

    float total = 0.0f;
    for(auto&& [id, light] : lights.directionals) {
        unused(id);
        total += occupancy(light->shadow_lod()) * light->cascades();
    }

    for(auto&& [id, transform, light] : lights.spots) {
        unused(id, transform);
        total += occupancy(light->shadow_lod());
    }
    return total;
}



ShadowMapPass ShadowMapPass::create(FrameGraph& framegraph, const SceneView& scene, const ShadowMapSettings& settings) {
    const auto region = framegraph.region("Shadows");

    static constexpr ImageFormat shadow_format = VK_FORMAT_D32_SFLOAT;

    FrameGraphPassBuilder builder = framegraph.add_pass("Shadow pass");
    const ecs::EntityWorld& world = scene.world();

    const u32 shadow_map_log_size = log2ui(settings.shadow_map_size);
    const u32 first_level_size = 1 << shadow_map_log_size;
    if(first_level_size != settings.shadow_map_size) {
        log_msg("Shadow map size is not a power of two", Log::Warning);
    }

    const math::Vec2ui shadow_map_size = math::Vec2ui(1, settings.shadow_atlas_size) * first_level_size;
    const math::Vec2 uv_mul = 1.0f / math::Vec2(shadow_map_size);

    const auto shadow_map = builder.declare_image(shadow_format, shadow_map_size);

    const ShadowCastingLights lights = collect_shadow_casting_lights(scene);

    const float downsample_factor = settings.spill_policy == ShadowMapSpillPolicy::DownSample
        ? total_occupancy(lights) / settings.shadow_atlas_size
        : 1.0f;
    const u32 lod_offset = log2ui(u32(std::ceil(downsample_factor)));

    ShadowMapPass pass;
    pass.shadow_map = shadow_map;
    pass.shadow_indices = std::make_shared<core::FlatHashMap<u64, math::Vec4ui>>();

    core::Vector<SubPass> sub_passes;
    {
        SubAtlasAllocator allocator(first_level_size);

        for(auto&& [id, light] : lights.directionals) {
            auto& indices = (*pass.shadow_indices)[id.as_u64()];
            indices = math::Vec4ui(u32(-1));

            const usize cascades = light->cascades();
            const float cascade_ratio = std::max(2.0f, light->last_cascade_distance() / light->first_cascade_distance());
            const float cascade_dist_mul = cascades > 1 ? std::exp(std::log(cascade_ratio) / (cascades - 1)) : 1.0f;

            float dist_mul = 1.0f;
            float near_dist = 0.0f;
            for(usize i = 0; i != cascades; ++i) {
                const float cascade_dist = light->first_cascade_distance() * dist_mul;
                dist_mul *= cascade_dist_mul;

                const u32 level = light->shadow_lod() + lod_offset;
                const auto [offset, size] = allocator.alloc(level);

                indices[i] = u32(sub_passes.size());
                const Camera light_cam = directional_camera(scene.camera(), *light, size, near_dist, cascade_dist);
                sub_passes.emplace_back(create_sub_pass(builder, offset, size, SceneView(&world, light_cam), uv_mul));

                near_dist = cascade_dist;
            }
        }

        for(auto&& [id, transform, light] : lights.spots) {
            auto& indices = (*pass.shadow_indices)[id.as_u64()];
            indices = math::Vec4ui(u32(-1));

            const u32 level = light->shadow_lod() + lod_offset;
            const auto [offset, size] = allocator.alloc(level);

            indices[0] = u32(sub_passes.size());
            sub_passes.emplace_back(create_sub_pass(builder, offset, size, SceneView(&world, spotlight_camera(*transform, *light)), uv_mul));
        }
    }

    const auto shadow_buffer = builder.declare_typed_buffer<uniform::ShadowMapParams>(sub_passes.size());
    pass.shadow_params = shadow_buffer;

    builder.map_buffer(shadow_buffer);
    builder.add_depth_output(shadow_map);
    builder.set_render_func([=, passes = std::move(sub_passes)](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        auto shadow_params = self->resources().map_buffer(shadow_buffer);

        for(usize i = 0; i != passes.size(); ++i) {
            const auto& pass = passes[i];
            shadow_params[i] = pass.params;

            render_pass.set_viewport(Viewport(math::Vec2(float(pass.viewport_size)), pass.viewport_offset));
            pass.scene_pass.render(render_pass, self);
        }
    });


    return pass;
}

}

