/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#include <yave/framegraph/FrameGraphPassBuilder.h>

#include <yave/ecs/EntityWorld.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/entities/entities.h>

#include <editor/context/EditorContext.h>
#include <editor/ui/ImGuiRenderer.h>

#include <imgui/yave_imgui.h>

// we actually need this to index utf-8 chars from the imgui font
IMGUI_API int ImTextCharFromUtf8(unsigned int* out_char, const char* in_text, const char* in_text_end);


// mostly copied from SceneRednerSubPass
namespace editor {

static constexpr usize max_batch_size = 128 * 1024;

struct ImGuiBillboardVertex {
	math::Vec3 position;
	math::Vec2 uv;
	math::Vec2 size;
};
static_assert(sizeof(ImGuiBillboardVertex) == (3 + 2 + 2) * sizeof(float));

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


static void render_editor_entities(ContextPtr ctx,
								   RenderPassRecorder& recorder, const FrameGraphPass* pass,
								   const SceneView& scene_view,
								   FrameGraphMutableTypedBufferId<EditorPassData> pass_buffer,
								   FrameGraphMutableTypedBufferId<ImGuiBillboardVertex> vertex_buffer) {

	y_profile();


	{
		auto mapping = pass->resources()->mapped_buffer(pass_buffer);
		mapping->view_proj = scene_view.camera().viewproj_matrix();
		mapping->viewport_size = pass->framebuffer().size();
		mapping->size = 64.0f;

	}

	{
		auto vertices = pass->resources()->buffer<BufferUsage::AttributeBit>(vertex_buffer);
		recorder.bind_attrib_buffers({vertices, vertices});
	}

	{
		const auto* material = ctx->resources()[EditorResources::ImGuiBillBoardMaterialTemplate];
		recorder.bind_material(material, {pass->descriptor_sets()[0]});
	}

	{
		auto [uv, size] = compute_uv_size(ICON_FA_LIGHTBULB);

		usize index = 0;
		auto mapping = pass->resources()->mapped_buffer(vertex_buffer);
		for(const auto& [tr, li] : scene_view.world().view(PointLightArchetype())) {
			math::Vec3 pos = tr.position();
			mapping[index++] = ImGuiBillboardVertex{pos, uv, size};
		}
		if(index) {
			recorder.draw(vk::DrawIndirectCommand(index, 1));
		}
	}

}


EditorRenderer EditorRenderer::create(ContextPtr ctx, FrameGraph& framegraph, const SceneView& view, const math::Vec2ui& size, const std::shared_ptr<IBLData>& ibl_data, const Settings& settings) {
	EditorRenderer renderer;
	renderer.renderer = DefaultRenderer::create(framegraph, view, size, ibl_data);

	auto pass_buffer = framegraph.declare_typed_buffer<EditorPassData>();
	auto vertex_buffer = framegraph.declare_typed_buffer<ImGuiBillboardVertex>(max_batch_size);

	FrameGraphPassBuilder builder = framegraph.add_pass("Editor pass");
	builder.add_uniform_input(ctx->ui().renderer().font_texture());
	builder.add_uniform_input(pass_buffer);

	builder.add_attrib_input(vertex_buffer);
	builder.map_update(pass_buffer);
	builder.map_update(vertex_buffer);

	builder.add_depth_output(renderer.renderer.gbuffer.depth, Framebuffer::LoadOp::Load);
	builder.add_color_output(renderer.renderer.tone_mapping.tone_mapped, Framebuffer::LoadOp::Load);
	builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			auto render_pass = recorder.bind_framebuffer(self->framebuffer());
			if(settings.enable_editor_entities) {
				render_editor_entities(ctx, render_pass, self, view, pass_buffer, vertex_buffer);
			}
		});


	return renderer;
}

}
