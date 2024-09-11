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
#include <yave/scene/EcsScene.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/StaticMeshComponent.h>

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

    const Scene* scene = scene_view.scene();

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

        auto push_entity = [&](const auto* tr) {
            vertex_mapping[index] = ImGuiBillboardVertex{scene->transform(*tr).position(), uv, size, tr->entity_index};
            ++index;
        };

        {
            std::tie(uv, size) = imgui::compute_glyph_uv_size(ICON_FA_LIGHTBULB);
            for(const PointLightObject* light : visibility.visible->point_lights) {
                push_entity(light);
            }
        }

        {
            std::tie(uv, size) = imgui::compute_glyph_uv_size(ICON_FA_VIDEO);
            for(const SpotLightObject* light : visibility.visible->spot_lights) {
                push_entity(light);
            }
        }

        if(index) {
            recorder.draw_array(index);
        }
    }
}

static void render_selection(DirectDraw& draw, const SceneView& scene_view) {
    const EditorWorld& world = current_world();
    const EcsScene* scene = dynamic_cast<const EcsScene*>(scene_view.scene());

    if(!scene) {
        return;
    }

    if(const ecs::EntityId selected = world.selected_entity(); selected.is_valid()) {

        if(const PointLightObject* obj = scene->point_light(selected)) {
            const math::Transform<> tr = scene->transform(*obj);
            draw.add_primitive("selected light")->add_sphere_3circles(tr.position(), tr.scale().max_component() * obj->component.range());
        }

        if(const SpotLightObject* obj = scene->spot_light(selected)) {
            const math::Transform<> tr = scene->transform(*obj);
            const u32 inner_color = pack_to_u32(math::Vec4(1.0f, 1.0f, 0.0f, 0.25f));
            const float scale = tr.scale().max_component();
            const auto sphere = obj->component.enclosing_sphere();
            draw.add_primitive("selected light outer")->add_cone(tr.position(), tr.forward(), tr.up(), obj->component.range() * scale, obj->component.half_angle());
            draw.add_primitive("selected light inner", inner_color)->add_cone(tr.position(), tr.forward(), tr.up(), obj->component.range() * scale, obj->component.half_inner_angle());
            // draw.add_primitive("selected light sphere")->add_sphere_3circles(tr.transform_point(math::Vec3(0.0f, sphere.dist_to_center, 0.0f)), sphere.radius * scale);
        }

        if(app_settings().debug.display_selected_bbox) {
            if(const StaticMeshObject* obj = scene->mesh(selected)) {
                draw.add_primitive("selected bbox")->add_box(obj->global_aabb);
            }
            if(const PointLightObject* obj = scene->point_light(selected)) {
                draw.add_primitive("selected bbox")->add_box(obj->global_aabb);
            }
            if(const SpotLightObject* obj = scene->spot_light(selected)) {
                draw.add_primitive("selected bbox")->add_box(obj->global_aabb);
            }
        }
    }
}




static FrameGraphMutableImageId copy_or_dummy(FrameGraphPassBuilder& builder, FrameGraphImageId in, ImageFormat format, const math::Vec2ui& size) {
    if(in.is_valid()) {
        return builder.declare_copy(in);
    }
    return builder.declare_image(format, size);
}




EditorPass EditorPass::create(FrameGraph& framegraph, const SceneView& scene_view, const SceneVisibilitySubPass& visibility, FrameGraphImageId in_depth, FrameGraphImageId in_color, FrameGraphImageId in_id) {
    const math::Vec2ui size = framegraph.image_size(in_depth);

    FrameGraphPassBuilder builder = framegraph.add_pass("Editor entity pass");

    const Scene* scene = scene_view.scene();
    const usize buffer_size = scene->transform_manager().transform_buffer().size();

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
        render_editor_entities(render_pass, self, scene_view, visibility, pass_buffer, vertex_buffer);

        DirectDraw direct;
        render_selection(direct, scene_view);
        direct.render(render_pass, scene_view.camera().view_proj_matrix());
    });

    EditorPass pass;
    pass.depth = depth;
    pass.color = color;
    pass.id = id;
    return pass;
}

}

