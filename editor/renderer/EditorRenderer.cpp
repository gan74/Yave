/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include <yave/framegraph/FrameGraph.h>
#include <yave/framegraph/FrameGraphPass.h>
#include <yave/framegraph/FrameGraphFrameResources.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

#include <editor/context/EditorContext.h>

namespace editor {

static FrameGraphImageId render_selection_outline(ContextPtr ctx, FrameGraph& framegraph, FrameGraphImageId color, FrameGraphImageId id) {
    const ecs::EntityId selected = ctx->selection().selected_entity();
    if(!selected.is_valid()) {
        return color;
    }

    FrameGraphPassBuilder builder = framegraph.add_pass("Selection pass");

    const auto selection = builder.declare_copy(color);
    const auto params_buffer = builder.declare_typed_buffer<u32>();

    builder.add_color_output(selection);
    builder.add_uniform_input(id, 0, PipelineStage::FragmentBit);
    builder.add_uniform_input(params_buffer, 0, PipelineStage::FragmentBit);
    builder.map_update(params_buffer);
    builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
        self->resources().mapped_buffer(params_buffer)[0] = selected.index();
        auto render_pass = recorder.bind_framebuffer(self->framebuffer());
        const auto* material = ctx->resources()[EditorResources::SelectionMaterialTemplate];
        render_pass.bind_material(material, {self->descriptor_sets()[0]});
        render_pass.draw_array(3);
    });

    return selection;
}

EditorRenderer EditorRenderer::create(ContextPtr ctx, FrameGraph& framegraph, const SceneView& view, const math::Vec2ui& size, const EditorRendererSettings& settings) {
    y_profile();

    EditorRenderer renderer;
    renderer.renderer = DefaultRenderer::create(framegraph, view, size, settings.renderer_settings);
    renderer.id_pass = IdBufferPass::create(ctx, framegraph, view, size);

    renderer.color = renderer.renderer.color;
    renderer.depth = renderer.renderer.depth;

    if(settings.show_selection) {
        renderer.color = render_selection_outline(ctx, framegraph, renderer.color, renderer.id_pass.id);
    }

    if(settings.show_editor_entities) {
        renderer.entity_pass = EditorEntityPass::create(ctx, framegraph, view, renderer.depth, renderer.color);
        renderer.color = renderer.entity_pass.color;
    }

    return renderer;
}

}

