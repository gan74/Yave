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

#include "EditorPass.h"

#include <editor/Settings.h>
#include <editor/Selection.h>
#include <editor/EditorWorld.h>
#include <editor/EditorResources.h>
#include <editor/ImGuiPlatform.h>

#include <yave/graphics/buffers/buffers.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <yave/ecs/ecs.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/systems/OctreeSystem.h>

#include <external/imgui/yave_imgui.h>

#include <yave/utils/entities.h>

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

static std::pair<math::Vec2, math::Vec2> compute_uv_size(const char* c) {
    math::Vec2 uv;
    math::Vec2 size(1.0f);

    unsigned u = 0;
    ImTextCharFromUtf8(&u, c, c + std::strlen(c));
    if(const ImFontGlyph* glyph = ImGui::GetFont()->FindGlyph(u)) {
        uv = math::Vec2{glyph->U0, glyph->V0};
        size = math::Vec2{glyph->U1, glyph->V1} - uv;
    }
    return {uv, size};
}


static void add_circle(core::Vector<math::Vec3>& points, const math::Vec3& position, math::Vec3 x, math::Vec3 y, float radius = 1.0f, usize divs = 64) {
    x *= radius;
    y *= radius;
    const float seg_ang_size = (1.0f / divs) * 2.0f * math::pi<float>;

    math::Vec3 last = position + y;
    for(usize i = 1; i != divs + 1; ++i) {
        const math::Vec2 c(std::sin(i * seg_ang_size), std::cos(i * seg_ang_size));
        points << last;
        last = (position + (x * c.x()) + (y * c.y()));
        points << last;
    }
}

static void add_cone(core::Vector<math::Vec3>& points, const math::Vec3& position, math::Vec3 x, math::Vec3 y, float len, float angle, usize divs = 8, usize circle_subdivs = 8) {
    const math::Vec3 z = x.cross(y).normalized();

    const usize beg = points.size();
    add_circle(points, position + x * (std::cos(angle) * len), y, z, std::sin(angle) * len, circle_subdivs * divs);

    for(usize i = 0; i != divs; ++i) {
        points << points[beg + (i * 2) * circle_subdivs];
        points << position;
    }
}

static void add_box(core::Vector<math::Vec3>& points, const math::Transform<>& transform, const AABB& aabb) {
    const math::Vec3 size = aabb.half_extent();
    const std::array corners = {
        size,
        math::Vec3(-size.x(), -size.y(), size.z()),
        math::Vec3(size.x(), -size.y(), -size.z()),
        math::Vec3(-size.x(), size.y(), -size.z()),
    };
    const math::Vec3 center = aabb.center();
    for(const math::Vec3 a : corners) {
        for(usize i = 0; i != 3; ++i) {
            math::Vec3 b = a;
            b[i] *= -1.0f;
            points << transform.to_global(center + a) << transform.to_global(center + b);
        }
    }
}


static void render_selection(RenderPassRecorder& recorder,
                             const FrameGraphPass* pass,
                             const SceneView& scene_view) {

    const ecs::EntityWorld& world = scene_view.world();
    const ecs::EntityId selected = selection().selected_entity();

    const TransformableComponent* tr = world.component<TransformableComponent>(selected);
    if(!tr) {
        return;
    }

    constexpr bool draw_enclosing_sphere = false;
    const bool draw_bbox = app_settings().debug.display_selected_bbox;

    core::Vector<math::Vec3> points;
    {
        const math::Vec3 z = tr->up();
        const math::Vec3 y = tr->right();
        const math::Vec3 x = tr->forward();
        if(const auto* l = world.component<PointLightComponent>(selected)) {
            add_circle(points, tr->position(), x, y, l->radius());
            add_circle(points, tr->position(), y, z, l->radius());
            add_circle(points, tr->position(), z, x, l->radius());
        }

        if(const auto* l = world.component<SpotLightComponent>(selected)) {
            add_cone(points, tr->position(), x, y, l->radius(), l->half_angle());

            if(draw_enclosing_sphere) {
                const auto enclosing = l->enclosing_sphere();
                const math::Vec3 center = tr->position() + tr->forward() * enclosing.dist_to_center;
                add_circle(points, center, x, y, enclosing.radius);
                add_circle(points, center, y, z, enclosing.radius);
                add_circle(points, center, z, x, enclosing.radius);
            }
        }

        if(const auto* m = world.component<StaticMeshComponent>(selected)) {
            if(draw_bbox) {
                add_box(points, tr->transform(), m->aabb());
                add_box(points, math::Transform<>(), tr->to_global(m->aabb()));
            }
        }
    }

    if(!points.is_empty()) {
        TypedAttribBuffer<math::Vec3, MemoryType::CpuVisible> vertices(points.size());
        TypedMapping<math::Vec3> mapping(vertices);
        std::copy(points.begin(), points.end(), mapping.begin());

        const auto* material = resources()[EditorResources::WireFrameMaterialTemplate];
        recorder.bind_material(material, {pass->descriptor_sets()[0]});
        recorder.bind_attrib_buffers(vertices);
        recorder.draw_array(points.size());
    }
}

static void visit_octree(const OctreeNode& node, core::Vector<math::Vec3>& points) {
    if(node.is_empty()) {
        return;
    }

    add_box(points, math::Transform<>(), node.strict_aabb());

    for(const OctreeNode& c : node.children()) {
        visit_octree(c, points);
    }
}

static void render_octree(RenderPassRecorder& recorder,
                          const FrameGraphPass* pass,
                          const SceneView& scene_view) {

    const ecs::EntityWorld& world = scene_view.world();
    const OctreeSystem* octree = world.find_system<OctreeSystem>();

    core::Vector<math::Vec3> points;
    visit_octree(octree->root(), points);

    if(!points.is_empty()) {
        TypedAttribBuffer<math::Vec3, MemoryType::CpuVisible> vertices(points.size());
        TypedMapping<math::Vec3> mapping(vertices);
        std::copy(points.begin(), points.end(), mapping.begin());

        const auto* material = resources()[EditorResources::WireFrameMaterialTemplate];
        recorder.bind_material(material, {pass->descriptor_sets()[0]});
        recorder.bind_attrib_buffers(vertices);
        recorder.draw_array(points.size());
    }
}


static void render_editor_entities(RenderPassRecorder& recorder, const FrameGraphPass* pass,
                                   const SceneView& scene_view,
                                   const FrameGraphMutableTypedBufferId<EditorPassData> pass_buffer,
                                   FrameGraphMutableTypedBufferId<ImGuiBillboardVertex> vertex_buffer) {

    y_profile();

    const ecs::EntityWorld& world = scene_view.world();

    {
        auto mapping = pass->resources().mapped_buffer(pass_buffer);
        mapping->view_proj = scene_view.camera().viewproj_matrix();
        mapping->viewport_size = pass->framebuffer().size();
        mapping->size = 64.0f;
    }

    {
        const auto vertices = pass->resources().buffer<BufferUsage::AttributeBit>(vertex_buffer);
        recorder.bind_attrib_buffers(vertices);
    }

    {
        const auto* material = resources()[EditorResources::ImGuiBillBoardMaterialTemplate];
        recorder.bind_material(material, {pass->descriptor_sets()[0]});
    }

    {
        math::Vec2 uv;
        math::Vec2 size;

        usize index = 0;
        auto vertex_mapping = pass->resources().mapped_buffer(vertex_buffer);

        auto push_entity = [&](ecs::EntityId id) {
                if(const TransformableComponent* tr = world.component<TransformableComponent>(id)) {
                    vertex_mapping[index] = ImGuiBillboardVertex{tr->position(), uv, size, id.index()};
                    ++index;
                }
            };

        {
            std::tie(uv, size) = compute_uv_size(ICON_FA_LIGHTBULB);
            for(ecs::EntityId id : world.component_ids<PointLightComponent>()) {
                push_entity(id);
            }
        }

        {
            std::tie(uv, size) = compute_uv_size(ICON_FA_VIDEO);
            for(ecs::EntityId id : world.component_ids<SpotLightComponent>()) {
                push_entity(id);
            }
        }

        if(index) {
            recorder.draw_array(index);
        }
    }
}

static FrameGraphMutableImageId copy_or_dummy(FrameGraphPassBuilder& builder, FrameGraphImageId in, ImageFormat format, const math::Vec2ui& size) {
    if(in.is_valid()) {
        return builder.declare_copy(in);
    }
    return builder.declare_image(format, size);
}



EditorPass EditorPass::create( FrameGraph& framegraph, const SceneView& view, FrameGraphImageId in_depth, FrameGraphImageId in_color, FrameGraphImageId in_id) {
    const math::Vec2ui size = framegraph.image_size(in_depth);

    FrameGraphPassBuilder builder = framegraph.add_pass("Editor entity pass");

    auto pass_buffer = builder.declare_typed_buffer<EditorPassData>();
    const auto vertex_buffer = builder.declare_typed_buffer<ImGuiBillboardVertex>(max_batch_size);
    const auto depth = builder.declare_copy(in_depth);
    const auto color = copy_or_dummy(builder, in_color, VK_FORMAT_R8G8B8A8_UNORM, size);
    const auto id = copy_or_dummy(builder, in_id, VK_FORMAT_R32_UINT, size);

    builder.add_external_input(imgui_platform()->renderer()->font_texture());
    builder.add_uniform_input(pass_buffer);

    builder.add_attrib_input(vertex_buffer);

    builder.map_update(pass_buffer);
    builder.map_update(vertex_buffer);

    builder.add_depth_output(depth);
    builder.add_color_output(color);
    builder.add_color_output(id);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
            auto render_pass = recorder.bind_framebuffer(self->framebuffer());
            render_editor_entities(render_pass, self, view, pass_buffer, vertex_buffer);

            if(selection().has_selected_entity()) {
                render_selection(render_pass, self, view);
            }

            if(app_settings().debug.display_octree) {
                render_octree(render_pass, self, view);
            }
        });

    EditorPass pass;
    pass.depth = depth;
    pass.color = color;
    pass.id = id;
    return pass;
}

}

