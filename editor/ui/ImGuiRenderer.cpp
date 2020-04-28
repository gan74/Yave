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

#include "ImGuiRenderer.h"

#include <editor/context/EditorContext.h>
#include <yave/graphics/buffers/TypedWrapper.h>
#include <yave/framegraph/FrameGraph.h>

#include <imgui/yave_imgui.h>

#include <y/core/Chrono.h>
#include <y/io2/File.h>


namespace editor {

static const char* find_font(std::string_view name) {
	const std::array font_paths = {".", "..", "./fonts", "../fonts"};
	for(const auto path : font_paths) {
		const char* file = fmt_c_str("%/%", path, name);
		if(io2::File::open(file).is_ok()) {
			return file;
		}
	}
	return nullptr;
}

static ImageData load_font() {
	y_profile();

	ImGuiIO& io = ImGui::GetIO();

	// https://skia.googlesource.com/external/github.com/ocornut/imgui/+/v1.50/extra_fonts/README.txt
#if 0
	if(const char* font = find_font("Roboto-Regular.ttf")) {
		io.Fonts->AddFontFromFileTTF(font, 14.0f);
	} else {
		io.Fonts->AddFontDefault();
		log_msg("Roboto-Regular.ttf not found.", Log::Error);
	}
#else
	io.Fonts->AddFontDefault();
#endif

	if(const char* icons = find_font("fa-solid-900.ttf")) {
		ImFontConfig config;
		config.MergeMode = true;
		const ImWchar icon_ranges[] = {ICON_MIN_FA, ICON_MAX_FA, 0};
		io.Fonts->AddFontFromFileTTF(icons, 13.0f, &config, icon_ranges);
	} else {
		log_msg("fa-solid-900.ttf not found.", Log::Error);
	}

	u8* font_data = nullptr;
	int width = 0;
	int height = 0;
	io.Fonts->GetTexDataAsRGBA32(&font_data, &width, &height);
	return ImageData(math::Vec2ui(width, height), font_data, ImageFormat(vk::Format::eR8G8B8A8Unorm));
}

ImGuiRenderer::ImGuiRenderer(ContextPtr ctx) :
		ContextLinked(ctx),
		/*_index_buffer(imgui_index_buffer_size),
		_vertex_buffer(imgui_vertex_buffer_size),
		_uniform_buffer(device(), 1),*/
		_font(device(), load_font()),
		_font_view(_font) {

	ImGui::GetIO().Fonts->TexID = &_font_view;
	set_style(Style::Corporate3D);
}


const Texture& ImGuiRenderer::font_texture() const {
	return _font;
}

void ImGuiRenderer::set_style(Style st) {
	Style _style = st;
	{
#include "style.h"
	}
}

void ImGuiRenderer::render(RenderPassRecorder& recorder, const FrameToken&) {
	static_assert(sizeof(ImDrawVert) == sizeof(Vertex), "ImDrawVert is not of expected size");
	static_assert(sizeof(ImDrawIdx) == sizeof(u32), "16 bit indexes not supported");
	y_profile();

	const auto region = recorder.region("ImGui render", math::Vec4(0.7f, 0.7f, 0.7f, 1.0f));

	ImDrawData* draw_data = ImGui::GetDrawData();

	if(!draw_data) {
		return;
	}

	const auto next_power_of_2 = [](usize size) { return 2 << log2ui(size); };
	const usize imgui_index_buffer_size = next_power_of_2(ImGui::GetIO().MetricsRenderIndices);
	const usize imgui_vertex_buffer_size = next_power_of_2(ImGui::GetIO().MetricsRenderVertices);
	const TypedBuffer<u32, BufferUsage::IndexBit, MemoryType::CpuVisible> index_buffer(device(), imgui_index_buffer_size);
	const TypedBuffer<Vertex, BufferUsage::AttributeBit, MemoryType::CpuVisible> vertex_buffer(device(), imgui_vertex_buffer_size);
	const TypedUniformBuffer<math::Vec2> uniform_buffer(device(), 1);

	auto indexes = TypedMapping(index_buffer);
	auto vertices = TypedMapping(vertex_buffer);

	auto uniform = TypedMapping(uniform_buffer);
	uniform[0] = math::Vec2(ImGui::GetIO().DisplaySize);


	const auto create_descriptor_set = [&](const void* data) {
		const auto* tex = static_cast<const TextureView*>(data);
		return DescriptorSet(device(), {Descriptor(*tex, Sampler::Clamp), Descriptor(uniform_buffer)});
	};

	const DescriptorSetBase default_set = create_descriptor_set(&_font_view);

	const auto setup_state = [&](const void* tex) {
		y_profile_zone("setup state");
		const auto* material = context()->resources()[EditorResources::ImGuiMaterialTemplate];
		recorder.bind_material(material, {tex ? create_descriptor_set(tex) : default_set});
	};

	usize index_offset = 0;
	usize vertex_offset = 0;
	const void* current_tex = nullptr;

	recorder.bind_buffers(index_buffer, {vertex_buffer});
	for(auto c = 0; c != draw_data->CmdListsCount; ++c) {
		const ImDrawList* cmd_list = draw_data->CmdLists[c];

		if(cmd_list->IdxBuffer.Size + index_offset >= index_buffer.size()) {
			y_fatal("Index buffer overflow.");
		}

		if(cmd_list->VtxBuffer.Size + vertex_offset >= vertex_buffer.size()) {
			y_fatal("Vertex buffer overflow.");
		}

		std::copy(cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Data + cmd_list->IdxBuffer.Size, &indexes[index_offset]);
		std::copy(cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Data + cmd_list->VtxBuffer.Size, reinterpret_cast<ImDrawVert*>(&vertices[vertex_offset]));

		u32 drawn_index_offset = index_offset;
		for(auto i = 0; i != cmd_list->CmdBuffer.Size; ++i) {
			const ImDrawCmd& cmd = cmd_list->CmdBuffer[i];

			const vk::Offset2D offset(u32(cmd.ClipRect.x), u32(cmd.ClipRect.y));
			const vk::Extent2D extent(u32(cmd.ClipRect.z - cmd.ClipRect.x), u32(cmd.ClipRect.w - cmd.ClipRect.y));
			recorder.vk_cmd_buffer().setScissor(0, vk::Rect2D(offset, extent));

			if(cmd.UserCallback) {
				void* ptr = reinterpret_cast<void*>(cmd.UserCallback);
				reinterpret_cast<UIDrawCallback>(ptr)(recorder, cmd.UserCallbackData);
				current_tex = nullptr;
			}

			if(cmd.ElemCount) {
				if(current_tex != cmd.TextureId) {
					setup_state(current_tex = cmd.TextureId);
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


}
