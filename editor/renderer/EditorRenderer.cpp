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

#include "EditorRenderer.h"
#include "IdBufferPass.h"
#include "EditorPass.h"

#include <editor/EditorWorld.h>
#include <editor/EditorResources.h>

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>
#include <yave/utils/DirectDraw.h>


namespace editor {

static FrameGraphImageId render_selection_outline(FrameGraph& framegraph, FrameGraphImageId color, FrameGraphImageId depth, FrameGraphImageId selection_depth, FrameGraphImageId selection_id) {
    if(!current_world().has_selected_entities()) {
        return color;
    }

    FrameGraphPassBuilder builder = framegraph.add_pass("Selection pass");

    const auto selection = builder.declare_copy(color);

    builder.add_color_output(selection);
    builder.add_uniform_input(depth);
    builder.add_uniform_input(selection_depth);
    builder.add_uniform_input(selection_id);
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass* self) {
        const auto* material = resources()[EditorResources::SelectionMaterialTemplate];
        render_pass.bind_material_template(material, self->descriptor_sets());
        render_pass.draw_array(3);
    });

    return selection;
}

static FrameGraphImageId render_debug_drawer(FrameGraph& framegraph, const SceneView& view, FrameGraphImageId in_color, FrameGraphImageId in_depth) {
    FrameGraphPassBuilder builder = framegraph.add_pass("Debug drawer pass");

    const auto color = builder.declare_copy(in_color);
    const auto depth = builder.declare_copy(in_depth);

    builder.add_color_output(color);
    builder.add_depth_output(depth);
    builder.set_render_func([=](RenderPassRecorder& render_pass, const FrameGraphPass*) {
        debug_drawer().render(render_pass, view.camera().view_proj_matrix());
        debug_drawer().clear();
    });

    return color;
}


EditorRenderer EditorRenderer::create(FrameGraph& framegraph, const SceneView& view, const math::Vec2ui& size, const EditorRendererSettings& settings) {
    y_profile();

    EditorRenderer renderer;
    {
        const auto region = framegraph.region("Engine");
        renderer.renderer = DefaultRenderer::create(framegraph, view, size, settings.renderer_settings);
        renderer.final = renderer.renderer.final;
        renderer.depth = renderer.renderer.depth;
    }

    const SceneView& scene_view = renderer.renderer.camera.unjittered_view;

    if(settings.show_editor_entities) {
        const EditorPass ed = EditorPass::create(framegraph, scene_view, renderer.depth, renderer.final);
        renderer.depth = ed.depth;
        renderer.final = ed.color;
    }

    if(settings.show_selection) {
        auto id_and_depth = [](const auto& pass) { return std::pair{pass.id, pass.depth}; };
        const IdBufferPass id_pass = IdBufferPass::create(framegraph, scene_view, size, EditorPassFlags::SelectionAndChildren);
        const auto [id, depth] = settings.show_editor_entities
            ? id_and_depth(EditorPass::create(framegraph, scene_view, id_pass.depth, {}, id_pass.id, EditorPassFlags::SelectionAndChildren))
            : id_and_depth(id_pass);

        renderer.final = render_selection_outline(framegraph, renderer.final, renderer.depth, depth, id);
    }

    if(settings.show_debug_drawer) {
        renderer.final = render_debug_drawer(framegraph, scene_view, renderer.final, renderer.depth);
    }

    return renderer;
}

}

