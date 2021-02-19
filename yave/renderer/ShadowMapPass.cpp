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

#include <yave/ecs/EntityWorld.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/entities/entities.h>

#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <y/utils/log.h>

namespace yave {

struct SubAtlas {
    math::Vec2ui pos;
    u32 subs = 4;
};

static std::pair<math::Vec2ui, u32> alloc_sub_atlas(u32 level, usize& first_level_index, core::MutableSpan<SubAtlas> levels, usize first_level_size) {
    y_always_assert(level < levels.size(), "Invalid atlas level");
    const u32 size = first_level_size >> level;
    const u32 index = levels[level].subs;
    if(index != 4) {
        ++levels[level].subs;
        return {math::Vec2ui(index >> 1, index & 1) * size + levels[level].pos, size};
    }

    if(level == 0) {
        if(first_level_index < levels.size()) {
            return {math::Vec2ui(0, first_level_index++) * first_level_size, first_level_size};
        }
        return {math::Vec2ui(), 0};
    }

    const math::Vec2ui pos = alloc_sub_atlas(level - 1, first_level_index, levels, first_level_size).first;
    levels[level] = {pos, 0};
    return alloc_sub_atlas(level, first_level_index, levels, first_level_size);
}

static Camera spotlight_camera(const TransformableComponent& tr, const SpotLightComponent& sp) {
    Camera cam;
    cam.set_proj(math::perspective(sp.half_angle() * 2.0f, 1.0f, 0.1f));
    cam.set_view(math::look_at(tr.position(), tr.position() + tr.forward(), tr.up()));
    return cam;
}

ShadowMapPass ShadowMapPass::create(FrameGraph& framegraph, const SceneView& scene, const ShadowMapSettings& settings) {
    const auto region = framegraph.region("Shadows");

    static constexpr ImageFormat shadow_format = VK_FORMAT_D32_SFLOAT;
    const ecs::EntityWorld& world = scene.world();

    FrameGraphPassBuilder builder = framegraph.add_pass("Shadow pass");

    const usize shadow_map_log_size = log2ui(settings.shadow_map_size);
    const usize first_level_size = 1 << shadow_map_log_size;
    if(first_level_size != settings.shadow_map_size) {
        log_msg("Shadow map size is not a power of two", Log::Warning);
    }


    usize first_level_index = 0;
    std::array<SubAtlas, 32> levels;
    auto alloc = [&](u32 level) { return alloc_sub_atlas(level, first_level_index, core::MutableSpan(levels), first_level_size); };


    const math::Vec2ui shadow_map_size = math::Vec2ui(1, settings.shadow_atlas_size) * first_level_size;
    const auto shadow_map = builder.declare_image(shadow_format, shadow_map_size);

    ShadowMapPass pass;
    pass.shadow_map = shadow_map;
    pass.sub_passes = std::make_shared<SubPassData>();

    {
        const math::Vec2 uv_mul = 1.0f / math::Vec2(shadow_map_size);
        for(auto spot : world.view(SpotLightArchetype())) {
            auto [t, l] = spot.components();
            if(!l.cast_shadow()) {
                continue;
            }

            const u32 level = l.shadow_lod();
            const auto [offset, size] = alloc(level);

            if(!size) {
                log_msg("Unable to allocate shadow altas: too many shadow casters", Log::Warning);
                continue;
            }

            const SceneView spot_view(&world, spotlight_camera(t, l));
            pass.sub_passes->passes.push_back(SubPass{
                SceneRenderSubPass::create(builder, spot_view),
                offset, size
            });
            pass.sub_passes->lights[spot.id().as_u64()] = {
                spot_view.camera().viewproj_matrix(),
                math::Vec2(offset) * uv_mul,
                math::Vec2(size) * uv_mul,
                float(size),
                1.0f / float(size),
                {},
            };
        }
    }

    builder.add_depth_output(shadow_map);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        auto render_pass = recorder.bind_framebuffer(self->framebuffer());

        for(const auto& sub_pass : pass.sub_passes->passes) {
            render_pass.set_viewport(Viewport(math::Vec2(sub_pass.viewport_size), sub_pass.viewport_offset));
            sub_pass.scene_pass.render(render_pass, self);
        }
    });

    return pass;
}

}

