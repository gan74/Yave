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


#include <y/io/File.h>


namespace editor {

static constexpr usize imgui_index_buffer_size = 64 * 1024;
static constexpr usize imgui_vertex_buffer_size = 64 * 1024;

static ImageData load_font() {
	ImGuiIO& io = ImGui::GetIO();
	u8* font_data = nullptr;
	int width = 0;
	int height = 0;
	io.Fonts->GetTexDataAsRGBA32(&font_data, &width, &height);
	io.Fonts->TexID = reinterpret_cast<void*>(1);
	auto image = ImageData(math::Vec2ui(width, height), font_data, ImageFormat(vk::Format::eR8G8B8A8Unorm));
	return image;
}

static void setup_style() {
	// based on https://gist.github.com/ongamex/4ee36fb23d6c527939d0f4ba72144d29
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 0.0f;
	style.FrameRounding = 0.0f;
	style.ScrollbarRounding = 0.0f;

	style.GrabRounding = 0.0f;
	style.WindowRounding = 0.0f;
	style.ScrollbarRounding = 0.0f; // 3.0f
	style.FrameRounding = 0.0f; // 3.0f
	style.WindowTitleAlign = ImVec2(0.5f,0.5f);

	auto active_color = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);

	style.Colors[ImGuiCol_Text]                  = ImVec4(0.73f, 0.73f, 0.73f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.26f, 0.26f, 0.26f, 0.95f);
	style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	style.Colors[ImGuiCol_Border]                = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrab]         = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered]  = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrabActive]   = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.78f, 0.78f, 0.78f, 1.00f);
	style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
	style.Colors[ImGuiCol_Button]                = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_ButtonHovered]         = ImVec4(0.43f, 0.43f, 0.43f, 1.00f);
	style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);
	style.Colors[ImGuiCol_Header]                = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip]            = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_ResizeGripHovered]     = active_color;
	style.Colors[ImGuiCol_ResizeGripActive]      = active_color;
	style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.32f, 0.52f, 0.65f, 1.00f);
	style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);
}

ImGuiRenderer::ImGuiRenderer(DevicePtr dptr) :
		SecondaryRenderer(dptr),
		_index_buffer(device(), imgui_index_buffer_size),
		_vertex_buffer(device(), imgui_vertex_buffer_size),
		_uniform_buffer(device(), 1),
		_font(device(), load_font()),
		_material(device(), MaterialData()
			.set_frag_data(SpirVData::from_file(io::File::open("imgui.frag.spv").expected("Unable to load spirv file.")))
			.set_vert_data(SpirVData::from_file(io::File::open("imgui.vert.spv").expected("Unable to load spirv file.")))
			.set_bindings({Binding(_font), Binding(_uniform_buffer)})
			.set_depth_tested(false)
			.set_culled(false)
			.set_blended(true)
		) {

	setup_style();
}

ImGuiRenderer::~ImGuiRenderer() {
}

void ImGuiRenderer::render(RenderPassRecorder& recorder, const FrameToken&) {
	static_assert(sizeof(ImDrawVert) == sizeof(Vertex), "ImDrawVert is not of expected size");
	static_assert(sizeof(ImDrawIdx) == sizeof(u32), "16 bit indexes not supported");

	auto region = recorder.region("ImGuiRenderer::render");

	ImDrawData* draw_data = ImGui::GetDrawData();

	if(!draw_data) {
		return;
	}


	recorder.bind_buffers(SubBuffer<BufferUsage::IndexBit>(_index_buffer), {AttribSubBuffer<Vertex>(_vertex_buffer)});
	recorder.bind_material(_material);

	auto indexes = TypedMapping(_index_buffer);
	auto vertices = TypedMapping(_vertex_buffer);
	auto uniform = TypedMapping(_uniform_buffer);

	uniform[0] = math::Vec2(ImGui::GetIO().DisplaySize);


	usize index_offset = 0;
	usize vertex_offset = 0;
	for(auto i = 0; i != draw_data->CmdListsCount; ++i) {
		const ImDrawList* cmd_list = draw_data->CmdLists[i];

		if(cmd_list->IdxBuffer.Size + index_offset >= _index_buffer.size()) {
			fatal("Index buffer overflow.");
		}

		if(cmd_list->VtxBuffer.Size + vertex_offset >= _vertex_buffer.size()) {
			fatal("Vertex buffer overflow.");
		}

		std::copy(cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Data + cmd_list->IdxBuffer.Size, &indexes[index_offset]);
		std::copy(cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Data + cmd_list->VtxBuffer.Size, reinterpret_cast<ImDrawVert*>(&vertices[vertex_offset]));

		u32 drawn_index_offset = index_offset;
		for(auto i = 0; i != cmd_list->CmdBuffer.Size; ++i) {
			const ImDrawCmd& cmd = cmd_list->CmdBuffer[i];

			if(cmd.UserCallback) {
				fatal("User callback not supported.");
			}

			vk::Offset2D offset(cmd.ClipRect.x, cmd.ClipRect.y);
			vk::Extent2D extent(cmd.ClipRect.z - cmd.ClipRect.x, cmd.ClipRect.w - cmd.ClipRect.y);
			recorder.vk_cmd_buffer().setScissor(0, vk::Rect2D(offset, extent));

			recorder.draw(vk::DrawIndexedIndirectCommand()
					.setFirstIndex(drawn_index_offset)
					.setVertexOffset(vertex_offset)
					.setIndexCount(cmd.ElemCount)
					.setInstanceCount(1)
				);

			drawn_index_offset += cmd.ElemCount;
		}

		vertex_offset += cmd_list->VtxBuffer.Size;
		index_offset += cmd_list->IdxBuffer.Size;
	}
}

void ImGuiRenderer::build_frame_graph(FrameGraphNode&) {
}

}
