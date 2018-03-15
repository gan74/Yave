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
	io.MemFreeFn(font_data);
	return image;
}

ImGuiRenderer::ImGuiRenderer(DevicePtr dptr) :
		SecondaryRenderer(dptr),
		_index_buffer(device(), imgui_index_buffer_size),
		_vertex_buffer(device(), imgui_vertex_buffer_size),
		_font(device(), load_font()),
		_material(device(), MaterialData()
			.set_frag_data(SpirVData::from_file(io::File::open("imgui.frag.spv").expected("Unable to load spirv file.")))
			.set_vert_data(SpirVData::from_file(io::File::open("imgui.vert.spv").expected("Unable to load spirv file.")))
			.set_bindings({Binding(_font)})
			.set_depth_tested(false)
			.set_culled(false)
			.set_blended(true)
		) {
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

	auto indexes = TypedMapping(_index_buffer);
	auto vertices = TypedMapping(_vertex_buffer);

	//float height = recorder.viewport().extent.y();

	usize index_offset = 0;
	usize vertex_offset = 0;
	for(auto i = 0; i != draw_data->CmdListsCount; ++i) {
		const ImDrawList* cmd_list = draw_data->CmdLists[i];
		std::copy(cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Data + cmd_list->IdxBuffer.Size, &indexes[index_offset]);
		std::copy(cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Data + cmd_list->VtxBuffer.Size, reinterpret_cast<ImDrawVert*>(&vertices[vertex_offset]));

		u32 drawn_index_offset = index_offset;
		for(auto i = 0; i != cmd_list->CmdBuffer.Size; ++i) {
			const ImDrawCmd& cmd = cmd_list->CmdBuffer[i];

			if(cmd.UserCallback) {
				fatal("User callback not supported.");
			}


			/*vk::Offset2D offset(cmd.ClipRect.x, height - cmd.ClipRect.w);
			vk::Extent2D extent(cmd.ClipRect.z - cmd.ClipRect.x, cmd.ClipRect.w - cmd.ClipRect.y);
			recorder.vk_cmd_buffer().setScissor(0, vk::Rect2D(offset, extent));*/

			recorder.bind_material(_material);
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
