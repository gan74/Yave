/*******************************
Copyright (c) 2016-2021 Gr�goire Angerand

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

template<typename T>
static SubPass create_sub_pass(FrameGraphPassBuilder& builder,
                            const T& light,
                            const SceneView& light_view,
                            const math::Vec2& uv_mul,
                            SubAtlasAllocator& allocator) {
    y_profile();

    const u32 level = light.shadow_lod();
    const auto [offset, size] = allocator.alloc(level);

    if(!size) {
        log_msg("Unable to allocate shadow altas: too many shadow casters", Log::Warning);
    }

    const float size_f = float(size);
    const uniform::ShadowMapParams params = {
        light_view.camera().viewproj_matrix(),
        math::Vec2(offset) * uv_mul,
        uv_mul * size_f,
        size_f,
        1.0f / size_f,
        {},
    };

    return SubPass{
        SceneRenderSubPass::create(builder, light_view),
        offset, size,
        params
    };
}



static Camera spotlight_camera(const TransformableComponent& tr, const SpotLightComponent& light) {
    const float z_near = 0.1f;

    Camera camera;
    camera.set_view(math::look_at(tr.position(), tr.position() + tr.forward(), tr.up()));
    camera.set_proj(math::perspective(light.half_angle() * 2.0f, 1.0f, z_near));
    camera.set_far(light.radius());
    return camera;
}

static Camera directional_camera(const Camera& cam, const DirectionalLightComponent& light) {
    const math::Vec3 cam_fwd = cam.forward();
    const math::Matrix4<> inv_matrix = cam.inverse_matrix();

    std::array<math::Vec3, 8> corners;
    for(usize i = 0; i != 8; ++i) {
        const math::Vec3 ndc = math::Vec3((i / 4), (i / 2) % 2, i % 2) * 2.0f - 1.0f;
        const math::Vec4 pos = inv_matrix * math::Vec4(ndc, 1.0f);
        corners[i] = pos.to<3>() / pos.w();
    }

    math::Vec3 center;
    for(usize i = 0; i != 4; ++i) {
        math::Vec3& a = corners[i * 2];
        const math::Vec3 b = corners[i * 2 + 1];
        a = b - (a - b).normalized() * light.cascade_distance();
        center += a + b;
    }
    center /= 8.0f;

    const math::Vec3 up = light.direction().cross(cam_fwd);
    const math::Matrix4<> view = math::look_at(center, center - light.direction(), up);

    math::Vec3 max(-std::numeric_limits<float>::max());
    math::Vec3 min(std::numeric_limits<float>::max());
    for(const math::Vec3 c : corners) {
        const math::Vec4 light_space = view * math::Vec4(c, 1.0f);
        for(usize i = 0; i != 3; ++i) {
            max[i] = std::max(max[i], light_space[i]);
            min[i] = std::min(min[i], light_space[i]);
        }
    }

    const float z_factor = 1000.0f;
    const float inv_z_factor = 1.0f / z_factor;
    max.z() *= max.z() < 0.0f ? inv_z_factor : z_factor;
    min.z() *= min.z() > 0.0f ? inv_z_factor : z_factor;

    Camera camera;
    camera.set_view(view);
    camera.set_proj(math::ortho(min.x(), max.x(), min.y(), max.y(), min.z(), max.z()));
    return camera;
}

ShadowMapPass ShadowMapPass::create(FrameGraph& framegraph, const SceneView& scene, const ShadowMapSettings& settings) {
    const auto region = framegraph.region("Shadows");

    static constexpr ImageFormat shadow_format = VK_FORMAT_D32_SFLOAT;
    const ecs::EntityWorld& world = scene.world();

    FrameGraphPassBuilder builder = framegraph.add_pass("Shadow pass");

    const u32 shadow_map_log_size = log2ui(settings.shadow_map_size);
    const u32 first_level_size = 1 << shadow_map_log_size;
    if(first_level_size != settings.shadow_map_size) {
        log_msg("Shadow map size is not a power of two", Log::Warning);
    }


    const math::Vec2ui shadow_map_size = math::Vec2ui(1, settings.shadow_atlas_size) * first_level_size;
    const math::Vec2 uv_mul = 1.0f / math::Vec2(shadow_map_size);

    const auto shadow_map = builder.declare_image(shadow_format, shadow_map_size);

    ShadowMapPass pass;
    pass.shadow_map = shadow_map;
    pass.shadow_indexes = std::make_shared<core::FlatHashMap<u64, u32>>();

    core::Vector<SubPass> sub_passes;

    {
        SubAtlasAllocator allocator(first_level_size);

        for(auto light : world.query<DirectionalLightComponent>()) {
            const auto& [l] = light.components();
            if(!l.cast_shadow()) {
                continue;
            }

            (*pass.shadow_indexes)[light.id().as_u64()] = u32(sub_passes.size());

            sub_passes.emplace_back(create_sub_pass(builder, l, SceneView(&world, directional_camera(scene.camera(), l)), uv_mul, allocator));
        }

        for(auto light : world.query<TransformableComponent, SpotLightComponent>()) {
            const auto& [t, l] = light.components();
            if(!l.cast_shadow()) {
                continue;
            }

            (*pass.shadow_indexes)[light.id().as_u64()] = u32(sub_passes.size());

            sub_passes.emplace_back(create_sub_pass(builder, l, SceneView(&world, spotlight_camera(t, l)), uv_mul, allocator));
        }
    }

    const auto shadow_buffer = builder.declare_typed_buffer<uniform::ShadowMapParams>(sub_passes.size());
    pass.shadow_params = shadow_buffer;

    builder.map_buffer(shadow_buffer);
    builder.add_depth_output(shadow_map);
    builder.set_render_func([=, passes = std::move(sub_passes)](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        TypedMapping<uniform::ShadowMapParams> shadow_params = self->resources().map_buffer(shadow_buffer);

        auto render_pass = recorder.bind_framebuffer(self->framebuffer());
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

