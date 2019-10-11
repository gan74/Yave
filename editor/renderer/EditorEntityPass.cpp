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

#include "EditorEntityPass.h"

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

namespace editor {

struct ImGuiBillboardVertex {
	math::Vec3 position;
	math::Vec2 uv;
	math::Vec2 size;
	u32 entity_index;
};

struct EditorEntityPassData {
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

static void render_editor_entities(ContextPtr ctx, bool picking,
								   RenderPassRecorder& recorder, const FrameGraphPass* pass,
								   const SceneView& scene_view,
								   FrameGraphMutableTypedBufferId<EditorEntityPassData> pass_buffer,
								   FrameGraphMutableTypedBufferId<ImGuiBillboardVertex> vertex_buffer) {

	y_profile();

	const ecs::EntityWorld& world = scene_view.world();

	{
		auto mapping = pass->resources()->mapped_buffer(pass_buffer);
		mapping->view_proj = scene_view.camera().viewproj_matrix();
		mapping->viewport_size = pass->framebuffer().size();
		mapping->size = 64.0f;

	}

	{
		auto vertices = pass->resources()->buffer<BufferUsage::AttributeBit>(vertex_buffer);
		recorder.bind_attrib_buffers({vertices});
	}

	{
		const auto* material = ctx->resources()[picking
			? EditorResources::ImGuiBillBoardPickingMaterialTemplate
			: EditorResources::ImGuiBillBoardMaterialTemplate];
		recorder.bind_material(material, {pass->descriptor_sets()[0]});
	}

	{
		auto [uv, size] = compute_uv_size(ICON_FA_LIGHTBULB);

		usize index = 0;
		auto vertex_mapping = pass->resources()->mapped_buffer(vertex_buffer);

		auto push_entity = [&](ecs::EntityIndex entity_index) {
				if(const TransformableComponent* tr = world.component<TransformableComponent>(world.id_from_index(entity_index))) {
					vertex_mapping[index] = ImGuiBillboardVertex{tr->position(), uv, size, entity_index};
					++index;
				}
			};

		for(ecs::EntityIndex entity_index : world.indexes<PointLightComponent>()) {
			push_entity(entity_index);
		}

		if(index) {
			recorder.draw(vk::DrawIndirectCommand(index, 1));
		}
	}
}


EditorEntityPass EditorEntityPass::create(ContextPtr ctx, FrameGraph& framegraph, const SceneView& view, FrameGraphImageId in_depth, FrameGraphImageId in_color, bool picking) {
	FrameGraphPassBuilder builder = framegraph.add_pass("Editor entity pass");

	auto pass_buffer = builder.declare_typed_buffer<EditorEntityPassData>();
	auto vertex_buffer = builder.declare_typed_buffer<ImGuiBillboardVertex>(max_batch_size);
	auto depth = builder.declare_copy(in_depth);
	auto color = builder.declare_copy(in_color);

	EditorEntityPass pass;
	pass.depth = depth;
	(picking ? pass.id : pass.color) = color;

	builder.add_uniform_input(ctx->ui().renderer().font_texture());
	builder.add_uniform_input(pass_buffer);

	builder.add_attrib_input(vertex_buffer);

	builder.map_update(pass_buffer);
	builder.map_update(vertex_buffer);

	builder.add_depth_output(depth, Framebuffer::LoadOp::Load);
	builder.add_color_output(color, Framebuffer::LoadOp::Load);
	builder.set_render_func([=](CmdBufferRecorder& recorder, const FrameGraphPass* self) {
			auto render_pass = recorder.bind_framebuffer(self->framebuffer());
			render_editor_entities(ctx, picking, render_pass, self, view, pass_buffer, vertex_buffer);
		});

	return pass;
}

}
