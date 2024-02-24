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

#include "EditorPass.h"

#include <editor/Settings.h>
#include <editor/EditorWorld.h>
#include <editor/EditorResources.h>
#include <editor/ImGuiPlatform.h>

#include <yave/graphics/buffers/Buffer.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/graphics/device/DeviceResources.h>

#include <yave/ecs/ecs.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/systems/TransformableManagerSystem.h>

#include <yave/utils/DirectDraw.h>

#include <editor/utils/ui.h>

// we actually need this to index utf-8 chars from the imgui font (defined in imgui_internal)
IMGUI_API int ImTextCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end);


namespace editor {

struct ImGuiBillboardVertex {
    math::Vec3 position;
    math::Vec2 uv;
    math::Vec2 size;
    u32 entity_index;
};

struct EditorPassData {
    math::Matrix4<> view_proj;
    math::Vec2 viewport_size;
    float size;
};



static void render_editor_entities(RenderPassRecorder& recorder, const FrameGraphPass* pass,
                                   const SceneView& scene_view,
                                   const SceneVisibilitySubPass& visibility,
                                   const FrameGraphMutableTypedBufferId<EditorPassData> pass_buffer,
                                   FrameGraphMutableTypedBufferId<ImGuiBillboardVertex> vertex_buffer) {
    y_profile();

    const EditorWorld& world = current_world();
    y_debug_assert(&world == &scene_view.world());

    {
        auto mapping = pass->resources().map_buffer(pass_buffer);
        mapping->view_proj = scene_view.camera().view_proj_matrix();
        mapping->viewport_size = pass->framebuffer().size();
        mapping->size = mapping->viewport_size.min_component() / 16.0f;
    }

    {
        const auto vertices = pass->resources().buffer<BufferUsage::AttributeBit>(vertex_buffer);
        recorder.bind_attrib_buffers(vertices);
    }

    {
        const auto* material = resources()[EditorResources::ImGuiBillBoardMaterialTemplate];
        recorder.bind_material_template(material, pass->descriptor_sets());
    }

    {
        math::Vec2 uv;
        math::Vec2 size;

        usize index = 0;
        auto vertex_mapping = pass->resources().map_buffer(vertex_buffer);

        auto push_entity = [&](ecs::EntityId id) {
            if(const TransformableComponent* tr = world.component<TransformableComponent>(id)) {
                vertex_mapping[index] = ImGuiBillboardVertex{tr->position(), uv, size, id.index()};
                ++index;
            }
        };
        {
            std::tie(uv, size) = imgui::compute_glyph_uv_size(ICON_FA_LIGHTBULB);
            auto query = world.query<PointLightComponent>(*visibility.visible);
            for(ecs::EntityId id : query.ids()) {
                push_entity(id);
            }
        }

        {
            std::tie(uv, size) = imgui::compute_glyph_uv_size(ICON_FA_VIDEO);
            auto query = world.query<SpotLightComponent>(*visibility.visible);
            for(ecs::EntityId id : query.ids()) {
                push_entity(id);
            }
        }

        if(index) {
            recorder.draw_array(index);
        }
    }
}





static void render_selection_aabb(DirectDrawPrimitive* primitive, const SceneView& scene_view) {
    const EditorWorld& world = current_world();
    const ecs::EntityId selected = world.selected_entity();

    y_debug_assert(&scene_view.world() == &world);
    unused(scene_view);

    const TransformableComponent* tr = world.component<TransformableComponent>(selected);
    if(!tr || world.has_tag(selected, ecs::tags::hidden)) {
        return;
    }

    constexpr bool draw_enclosing_sphere = false;
    const bool draw_bbox = app_settings().debug.display_selected_bbox;
    const bool draw_octree = app_settings().debug.display_selected_octree;

    {
        const math::Vec3 z = tr->up().normalized();
        const math::Vec3 y = tr->right().normalized();
        const math::Vec3 x = tr->forward().normalized();
        const float scale = tr->transform().scale().max_component();

        auto add_sphere = [&](const math::Vec3& pos, float radius, const math::Vec3& color = math::Vec3(0.0f, 0.0f, 1.0f)) {
            primitive->set_color(color);
            primitive->add_circle(pos, x, y, radius);
            primitive->add_circle(pos, y, z, radius);
            primitive->add_circle(pos, z, x, radius);
        };

        if(const auto* l = world.component<PointLightComponent>(selected)) {
            add_sphere(tr->position(), l->range() * scale);
            add_sphere(tr->position(), l->min_radius() * scale, math::Vec3(1.0f, 1.0f, 0.0f));
        }

        if(const auto* l = world.component<SpotLightComponent>(selected)) {
            primitive->add_cone(tr->position(), x, y, l->range() * scale, l->half_angle());
            add_sphere(tr->position(), l->min_radius() * scale, math::Vec3(1.0f, 1.0f, 0.0f));

            if(draw_enclosing_sphere) {
                const auto enclosing = l->enclosing_sphere();
                const math::Vec3 center = tr->position() + tr->forward() * enclosing.dist_to_center * scale;
                add_sphere(center, enclosing.radius * scale);
            }
        }

        if(draw_bbox) {
            const u32 color = primitive->color();
            y_defer(primitive->set_color(color));

            primitive->set_color(math::Vec3(1, 1, 0));
            primitive->add_box(tr->global_aabb());
        }

        if(draw_octree && !tr->local_aabb().is_empty()) {
            if(const TransformableManagerSystem* tr_manager = world.find_system<TransformableManagerSystem>()) {
                const u32 color = primitive->color();
                y_defer(primitive->set_color(color));

                primitive->set_color(math::Vec3(1, 0, 0));
                primitive->add_box(tr_manager->parent_node_aabb(*tr));
            }
        }
    }
}


static void render_octree(DirectDrawPrimitive* primitive) {
    y_profile();

    const EditorWorld& world = current_world();
    const TransformableManagerSystem* tr_system = world.find_system<TransformableManagerSystem>();

    if(!tr_system) {
        return;
    }

    primitive->set_color(math::Vec3(0, 0, 1));
    for(const auto& node : tr_system->octree_nodes()) {
        primitive->add_box(node.strict_aabb());
    }

}


static FrameGraphMutableImageId copy_or_dummy(FrameGraphPassBuilder& builder, FrameGraphImageId in, ImageFormat format, const math::Vec2ui& size) {
    if(in.is_valid()) {
        return builder.declare_copy(in);
    }
    return builder.declare_image(format, size);
}






EditorPass EditorPass::create(FrameGraph& framegraph, const SceneView& view, const SceneVisibilitySubPass& visibility, FrameGraphImageId in_depth, FrameGraphImageId in_color, FrameGraphImageId in_id) {
    const math::Vec2ui size = framegraph.image_size(in_depth);

    FrameGraphPassBuilder builder = framegraph.add_pass("Editor entity pass");

    const usize buffer_size = view.world().entity_count();

    auto pass_buffer = builder.declare_typed_buffer<EditorPassData>();
    const auto vertex_buffer = builder.declare_typed_buffer<ImGuiBillboardVertex>(buffer_size);
    const auto depth = builder.declare_copy(in_depth);
    const auto color = copy_or_dummy(builder, in_color, VK_FORMAT_R8G8B8A8_UNORM, size);
    const auto id = copy_or_dummy(builder, in_id, VK_FORMAT_R32_UINT, size);

    builder.add_external_input(imgui_platform()->renderer()->font_texture());
    builder.add_uniform_input(pass_buffer);

    builder.add_attrib_input(vertex_buffer);

    builder.map_buffer(pass_buffer);
    builder.map_buffer(vertex_buffer);

    builder.add_depth_output(depth);
    builder.add_color_output(color);
    builder.add_color_output(id);
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        render_editor_entities(render_pass, self, view, visibility, pass_buffer, vertex_buffer);

        DirectDraw direct;
        {
            if(current_world().selected_entity().is_valid()) {
                render_selection_aabb(direct.add_primitive("selection"), view);
            }

            if(app_settings().debug.diplay_octree) {
                render_octree(direct.add_primitive("octree"));
            }
        }
        direct.render(render_pass, view.camera().view_proj_matrix());
    });

    EditorPass pass;
    pass.depth = depth;
    pass.color = color;
    pass.id = id;
    return pass;
}

}

