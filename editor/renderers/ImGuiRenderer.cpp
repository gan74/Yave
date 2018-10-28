/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include "ImGuiRenderer.h"

#include <yave/graphics/buffers/TypedWrapper.h>

#include <y/io/File.h>
#include <y/core/Chrono.h>

#include <imgui/imgui.h>

namespace editor {

static constexpr usize imgui_index_buffer_size = 64 * 1024;
static constexpr usize imgui_vertex_buffer_size = 64 * 1024;

static ImageData load_font() {
	core::DebugTimer _("load_font");

	// https://skia.googlesource.com/external/github.com/ocornut/imgui/+/v1.50/extra_fonts/README.txt
	ImGuiIO& io = ImGui::GetIO();

	io.Fonts->AddFontDefault();
	{
		ImFontConfig config;
		config.MergeMode = true;
		const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
		io.Fonts->AddFontFromFileTTF("fonts/fa-solid-900.ttf", 13.0f, &config, icon_ranges);
	}

	u8* font_data = nullptr;
	int width = 0;
	int height = 0;
	io.Fonts->GetTexDataAsRGBA32(&font_data, &width, &height);
	return ImageData(math::Vec2ui(width, height), font_data, ImageFormat(vk::Format::eR8G8B8A8Unorm));
}

template<typename = void>
static void setup_style() {

	ImGuiStyle& style = ImGui::GetStyle();
	style.ChildBorderSize = 0;
	style.PopupBorderSize = 0;
	style.FrameBorderSize = 0;
	style.TabBorderSize	= 0;

	style.PopupRounding = 0;
	style.WindowRounding = 0;
	style.ChildRounding = 0;
	style.FrameRounding = 3;
	style.TabRounding = 0;

	// style not complete yet

#include "style.h"
}

ImGuiRenderer::ImGuiRenderer(DevicePtr dptr) :
		SecondaryRenderer(dptr),
		_index_buffer(imgui_index_buffer_size),
		_vertex_buffer(imgui_vertex_buffer_size),
		_uniform_buffer(device(), 1),
		_font(device(), load_font()),
		_font_view(_font),
		_material(device(), MaterialData()
			.set_frag_data(SpirVData::deserialized(io::File::open("imgui.frag.spv").expected("Unable to load spirv file.")))
			.set_vert_data(SpirVData::deserialized(io::File::open("imgui.vert.spv").expected("Unable to load spirv file.")))
			.set_bindings({Binding(_uniform_buffer)})
			.set_depth_tested(false)
			.set_culled(false)
			.set_blended(true)
		) {

	ImGui::GetIO().Fonts->TexID = &_font_view;
	setup_style();
}

const DescriptorSet& ImGuiRenderer::create_descriptor_set(const void* data) {
	auto tex = reinterpret_cast<const TextureView*>(data);
	auto& ds = _descriptor_sets[tex->vk_view()];
	if(!ds.device()) {
		ds = DescriptorSet(device(), {Binding(*tex)});
	}
	return ds;
}

void ImGuiRenderer::setup_state(RenderPassRecorder& recorder, const FrameToken& token, const void* tex) {
	recorder.bind_buffers(_index_buffer[token], {_vertex_buffer[token]});
	if(tex) {
		recorder.bind_material(_material, {create_descriptor_set(tex)});
	} else {
		recorder.bind_material(_material);
	}
}

void ImGuiRenderer::render(RenderPassRecorder& recorder, const FrameToken&token) {
	static_assert(sizeof(ImDrawVert) == sizeof(Vertex), "ImDrawVert is not of expected size");
	static_assert(sizeof(ImDrawIdx) == sizeof(u32), "16 bit indexes not supported");

	auto region = recorder.region("ImGuiRenderer::render");

	ImDrawData* draw_data = ImGui::GetDrawData();

	if(!draw_data) {
		return;
	}

	auto index_subbuffer = _index_buffer[token];
	auto vertex_subbuffer = _vertex_buffer[token];
	auto indexes = TypedMapping(index_subbuffer);
	auto vertices = TypedMapping(vertex_subbuffer);
	auto uniform = TypedMapping(_uniform_buffer);

	uniform[0] = math::Vec2(ImGui::GetIO().DisplaySize);

	usize index_offset = 0;
	usize vertex_offset = 0;
	const void* current_tex = nullptr;
	for(auto i = 0; i != draw_data->CmdListsCount; ++i) {
		const ImDrawList* cmd_list = draw_data->CmdLists[i];

		if(cmd_list->IdxBuffer.Size + index_offset >= index_subbuffer.size()) {
			y_fatal("Index buffer overflow.");
		}

		if(cmd_list->VtxBuffer.Size + vertex_offset >= vertex_subbuffer.size()) {
			y_fatal("Vertex buffer overflow.");
		}

		std::copy(cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Data + cmd_list->IdxBuffer.Size, &indexes[index_offset]);
		std::copy(cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Data + cmd_list->VtxBuffer.Size, reinterpret_cast<ImDrawVert*>(&vertices[vertex_offset]));

		u32 drawn_index_offset = index_offset;
		for(auto i = 0; i != cmd_list->CmdBuffer.Size; ++i) {
			const ImDrawCmd& cmd = cmd_list->CmdBuffer[i];

			vk::Offset2D offset(cmd.ClipRect.x, cmd.ClipRect.y);
			vk::Extent2D extent(cmd.ClipRect.z - cmd.ClipRect.x, cmd.ClipRect.w - cmd.ClipRect.y);
			recorder.vk_cmd_buffer().setScissor(0, vk::Rect2D(offset, extent));

			if(cmd.UserCallback) {
				void* ptr = reinterpret_cast<void*>(cmd.UserCallback);
				reinterpret_cast<UIDrawCallback>(ptr)(recorder, cmd.UserCallbackData);
				current_tex = nullptr;
			}

			if(cmd.ElemCount) {
				if(current_tex != cmd.TextureId) {
					setup_state(recorder, token, current_tex = cmd.TextureId);
				}
				recorder.draw(vk::DrawIndexedIndirectCommand()
						.setFirstIndex(drawn_index_offset)
						.setVertexOffset(vertex_offset)
						.setIndexCount(cmd.ElemCount)
						.setInstanceCount(1)
					);

				drawn_index_offset += cmd.ElemCount;
			}
		}

		vertex_offset += cmd_list->VtxBuffer.Size;
		index_offset += cmd_list->IdxBuffer.Size;
	}
}

void ImGuiRenderer::build_frame_graph(FrameGraphNode&) {
}

}
