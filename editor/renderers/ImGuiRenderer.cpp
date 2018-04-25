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

#include <yave/buffers/TypedMapping.h>

#include <y/io/File.h>

#include <imgui/imgui.h>


namespace editor {

static constexpr usize imgui_index_buffer_size = 64 * 1024;
static constexpr usize imgui_vertex_buffer_size = 64 * 1024;

static ImageData load_font() {
	ImGuiIO& io = ImGui::GetIO();
	u8* font_data = nullptr;
	int width = 0;
	int height = 0;
	io.Fonts->GetTexDataAsRGBA32(&font_data, &width, &height);
	return ImageData(math::Vec2ui(width, height), font_data, ImageFormat(vk::Format::eR8G8B8A8Unorm));
}

template<typename = void>
static void setup_old_style() {
	// based on https://gist.github.com/ongamex/4ee36fb23d6c527939d0f4ba72144d29
	ImGuiStyle& style = ImGui::GetStyle();
	style.WindowRounding = 0.0f;
	style.FrameRounding = 0.0f;
	style.ScrollbarRounding = 0.0f;

	style.Alpha = 1.0f;
	style.GrabRounding = 0.0f;
	style.WindowRounding = 0.0f;
	style.ScrollbarRounding = 0.0f; // 3.0f
	style.FrameRounding = 0.0f; // 3.0f
	style.WindowTitleAlign = ImVec2(0.5f,0.5f);

	//style.WindowPadding = ImVec2(0.5f,0.5f);
	//style.ColumnsMinSpacing = 0.0f;

	//auto active_color = math::Vec4(0.26f, 0.59f, 0.98f, 1.00f);
	auto hovered_color = math::Vec4(0.43f, 0.43f, 0.43f, 1.00f);
	auto button_color = math::Vec4(0.36f, 0.36f, 0.36f, 1.00f);

	style.Colors[ImGuiCol_Text]                  = ImVec4(0.73f, 0.73f, 0.73f, 1.00f);
	style.Colors[ImGuiCol_TextDisabled]          = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
	style.Colors[ImGuiCol_WindowBg]              = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	style.Colors[ImGuiCol_PopupBg]               = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	style.Colors[ImGuiCol_Border]                = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	style.Colors[ImGuiCol_BorderShadow]          = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	style.Colors[ImGuiCol_MenuBarBg]             = ImVec4(0.26f, 0.26f, 0.26f, 1.00f);
	style.Colors[ImGuiCol_Header]                = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_HeaderHovered]         = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_HeaderActive]          = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_PlotLines]             = ImVec4(0.39f, 0.39f, 0.39f, 1.00f);
	style.Colors[ImGuiCol_PlotLinesHovered]      = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram]         = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_PlotHistogramHovered]  = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg]        = ImVec4(0.32f, 0.52f, 0.65f, 1.00f);
	style.Colors[ImGuiCol_ModalWindowDarkening]	 = ImVec4(0.20f, 0.20f, 0.20f, 0.50f);


	style.Colors[ImGuiCol_TitleBg]               = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_TitleBgCollapsed]      = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);
	style.Colors[ImGuiCol_TitleBgActive]         = ImVec4(0.36f, 0.36f, 0.36f, 1.00f);

	style.Colors[ImGuiCol_FrameBg]               = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered]        = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
	style.Colors[ImGuiCol_FrameBgActive]         = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);

	style.Colors[ImGuiCol_ScrollbarBg]           = ImVec4(0.21f, 0.21f, 0.21f, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrab]         = button_color;
	style.Colors[ImGuiCol_ScrollbarGrabHovered]  = hovered_color;
	style.Colors[ImGuiCol_ScrollbarGrabActive]   = hovered_color;

	style.Colors[ImGuiCol_CheckMark]             = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);

	style.Colors[ImGuiCol_SliderGrab]            = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);
	style.Colors[ImGuiCol_SliderGrabActive]      = ImVec4(0.74f, 0.74f, 0.74f, 1.00f);

	style.Colors[ImGuiCol_Button]                = button_color;
	style.Colors[ImGuiCol_ButtonHovered]         = hovered_color;
	style.Colors[ImGuiCol_ButtonActive]          = ImVec4(0.11f, 0.11f, 0.11f, 1.00f);

	style.Colors[ImGuiCol_ResizeGrip]            = button_color;
	style.Colors[ImGuiCol_ResizeGripHovered]     = hovered_color;
	style.Colors[ImGuiCol_ResizeGripActive]      = hovered_color;
}

/*static void setup_style() {
	// from https://github.com/volcoma/EtherealEngine/blob/e7ee49c403948cccb91f31e1022cee09235f5d96/editor/editor_runtime/interface/gui_system.cpp#L482
	auto col_main_hsv = math::Vec3(0.0f, 200.0f, 170.0f) / 255.0f;
	auto col_area_hsv = math::Vec3(0.0f, 0.0f, 80.0f) / 255.0f;
	auto col_back_hsv = math::Vec3(0.0f, 0.0f, 35.0f)  / 255.0f;
	auto col_text_hsv = math::Vec3(0.0f, 0.0f, 1.0f);

	ImVec4 col_text = ImColor::HSV(col_text_hsv.x(), col_text_hsv.y(), col_text_hsv.z());
	ImVec4 col_main = ImColor::HSV(col_main_hsv.x(), col_main_hsv.y(), col_main_hsv.z());
	ImVec4 col_back = ImColor::HSV(col_back_hsv.x(), col_back_hsv.y(), col_back_hsv.z());
	ImVec4 col_area = ImColor::HSV(col_area_hsv.x(), col_area_hsv.y(), col_area_hsv.z());

	ImGuiStyle& style = ImGui::GetStyle();
	style.FrameRounding = 0.0f;
	style.WindowRounding = 0.0f;

	style.Colors[ImGuiCol_Text] = ImVec4(col_text.x, col_text.y, col_text.z, 1.00f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(col_text.x, col_text.y, col_text.z, 0.58f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(col_back.x, col_back.y, col_back.z, 1.00f);
	//style.Colors[ImGuiCol_ChildBg] = ImVec4(col_area.x, col_area.y, col_area.z, 1.00f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(col_area.x * 0.8f, col_area.y * 0.8f, col_area.z * 0.8f, 1.00f);
	style.Colors[ImGuiCol_Border] = ImVec4(col_text.x, col_text.y, col_text.z, 0.30f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(col_back.x, col_back.y, col_back.z, 1.00f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(col_main.x, col_main.y, col_main.z, 0.68f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(col_main.x, col_main.y, col_main.z, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(col_main.x, col_main.y, col_main.z, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(col_main.x, col_main.y, col_main.z, 1.0f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(col_area.x, col_area.y, col_area.z, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(col_area.x, col_area.y, col_area.z, 1.00f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(col_main.x, col_main.y, col_main.z, 0.31f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(col_main.x, col_main.y, col_main.z, 0.78f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(col_text.x, col_text.y, col_text.z, 0.80f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(col_main.x, col_main.y, col_main.z, 0.54f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f);
	style.Colors[ImGuiCol_Button] = ImVec4(col_main.x, col_main.y, col_main.z, 0.44f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(col_main.x, col_main.y, col_main.z, 0.86f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f);
	style.Colors[ImGuiCol_Header] = ImVec4(col_main.x, col_main.y, col_main.z, 0.76f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(col_main.x, col_main.y, col_main.z, 0.86f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(col_main.x, col_main.y, col_main.z, 0.20f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(col_main.x, col_main.y, col_main.z, 0.78f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(col_text.x, col_text.y, col_text.z, 0.63f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(col_text.x, col_text.y, col_text.z, 0.63f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(col_main.x, col_main.y, col_main.z, 1.00f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(col_main.x, col_main.y, col_main.z, 0.43f);
	style.Colors[ImGuiCol_ModalWindowDarkening] = ImVec4(0.10f, 0.10f, 0.10f, 0.55f);
}*/

template<typename = void>
static void setup_style() {
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;

	style.GrabRounding = 0.0f;
	style.WindowRounding = 0.0f;
	style.ScrollbarRounding = 0.0f;
	style.FrameRounding = 0.0f;

	style.WindowTitleAlign = ImVec2(0.5f,0.5f);

	style.WindowBorderSize = 0.0f;
	//style.ChildBorderSize = 16.0f;

	auto dark_bg	= math::Vec4( 45,  45,  45, 255) / 255;
	auto bg			= math::Vec4( 65,  65,  65, 255) / 255;
	auto text		= math::Vec4(255, 255, 255, 255) / 255;

	auto debug		= math::Vec4(255, 0, 0, 255) / 255;
	unused(debug);

	/*for(int i = 0; i != ImGuiCol_COUNT; ++i) {
		style.Colors[i] = debug;
	}*/

	style.Colors[ImGuiCol_Text]                 = text;
	style.Colors[ImGuiCol_TextDisabled]         = text;
	style.Colors[ImGuiCol_WindowBg]             = dark_bg;

	style.Colors[ImGuiCol_FrameBg]              = dark_bg;
	style.Colors[ImGuiCol_FrameBgHovered]       = (dark_bg + bg) * 0.5f;
	style.Colors[ImGuiCol_FrameBgActive]        = bg;

	//style.Colors[ImGuiCol_Button]				= dark_bg;
	//style.Colors[ImGuiCol_ButtonActive]	        = dark_bg;
	//style.Colors[ImGuiCol_ButtonHovered]        = dark_bg;

	style.Colors[ImGuiCol_TitleBg]				= bg;
	style.Colors[ImGuiCol_TitleBgActive]		= bg;
	style.Colors[ImGuiCol_TitleBgCollapsed]		= bg;

	style.Colors[ImGuiCol_MenuBarBg]			= bg;

	style.Colors[ImGuiCol_ChildBg]              = ImVec4(); // tansparent

	style.Colors[ImGuiCol_Border]				= ImVec4();
	style.Colors[ImGuiCol_BorderShadow]			= ImVec4();


	//style.Colors[ImGuiCol_PopupBg]            = bg;
	//style.Colors[ImGuiCol_Border]             = bg;
	//style.Colors[ImGuiCol_BorderShadow]       = bg;
	//style.Colors[ImGuiCol_MenuBarBg]          = bg;
	//style.Colors[ImGuiCol_Header]             = bg;
	//style.Colors[ImGuiCol_HeaderHovered]      = bg;
	//style.Colors[ImGuiCol_HeaderActive]       = bg;
	//style.Colors[ImGuiCol_TextSelectedBg]     = bg;

	//style.Colors[ImGuiCol_TitleBg]            = bg;
	//style.Colors[ImGuiCol_TitleBgCollapsed]   = bg;
	//style.Colors[ImGuiCol_TitleBgActive]      = bg;
}

ImGuiRenderer::ImGuiRenderer(DevicePtr dptr) :
		SecondaryRenderer(dptr),
		_index_buffer(device(), imgui_index_buffer_size),
		_vertex_buffer(device(), imgui_vertex_buffer_size),
		_uniform_buffer(device(), 1),
		_font(device(), load_font()),
		_font_view(_font),
		_material(device(), MaterialData()
			.set_frag_data(SpirVData::from_file(io::File::open("imgui.frag.spv").expected("Unable to load spirv file.")))
			.set_vert_data(SpirVData::from_file(io::File::open("imgui.vert.spv").expected("Unable to load spirv file.")))
			.set_bindings({Binding(_uniform_buffer)})
			.set_depth_tested(false)
			.set_culled(false)
			.set_blended(true)
		) {

	ImGui::GetIO().Fonts->TexID = &_font_view;
	setup_old_style();
}

const DescriptorSet& ImGuiRenderer::create_descriptor_set(const void* data) {
	auto tex = reinterpret_cast<const TextureView*>(data);
	auto& ds = _descriptor_sets[tex->vk_view()];
	if(!ds.device()) {
		ds = DescriptorSet(device(), {Binding(*tex)});
	}
	return ds;
}

void ImGuiRenderer::setup_state(RenderPassRecorder& recorder, const void* tex) {
	recorder.bind_buffers(SubBuffer<BufferUsage::IndexBit>(_index_buffer), {AttribSubBuffer<Vertex>(_vertex_buffer)});
	if(tex) {
		recorder.bind_material(_material, {create_descriptor_set(tex)});
	} else {
		recorder.bind_material(_material);
	}
}

void ImGuiRenderer::render(RenderPassRecorder& recorder, const FrameToken&) {
	static_assert(sizeof(ImDrawVert) == sizeof(Vertex), "ImDrawVert is not of expected size");
	static_assert(sizeof(ImDrawIdx) == sizeof(u32), "16 bit indexes not supported");

	auto region = recorder.region("ImGuiRenderer::render");

	ImDrawData* draw_data = ImGui::GetDrawData();

	if(!draw_data) {
		return;
	}

	auto indexes = TypedMapping(_index_buffer);
	auto vertices = TypedMapping(_vertex_buffer);
	auto uniform = TypedMapping(_uniform_buffer);

	uniform[0] = math::Vec2(ImGui::GetIO().DisplaySize);

	usize index_offset = 0;
	usize vertex_offset = 0;
	const void* current_tex = nullptr;
	for(auto i = 0; i != draw_data->CmdListsCount; ++i) {
		const ImDrawList* cmd_list = draw_data->CmdLists[i];

		if(cmd_list->IdxBuffer.Size + index_offset >= _index_buffer.size()) {
			y_fatal("Index buffer overflow.");
		}

		if(cmd_list->VtxBuffer.Size + vertex_offset >= _vertex_buffer.size()) {
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
				reinterpret_cast<UIDrawCallback>(cmd.UserCallback)(recorder, cmd.UserCallbackData);
				current_tex = nullptr;
			}

			if(cmd.ElemCount) {
				if(current_tex != cmd.TextureId) {
					setup_state(recorder, current_tex = cmd.TextureId);
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
