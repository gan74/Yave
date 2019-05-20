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

#include "AssetStringifier.h"

#include <editor/context/EditorContext.h>

#include <imgui/yave_imgui.h>

namespace editor {

AssetStringifier::AssetStringifier(ContextPtr cptr) :
		Widget("Asset stringifier"),
		ContextLinked(cptr),
		_selector(cptr, AssetType::Mesh) {

	_selector.set_has_parent(true);
	_selector.set_selected_callback([this](AssetId id) { stringify(id); return true; });
}

void AssetStringifier::paint_ui(CmdBufferRecorder& recorder, const FrameToken& token) {
	{
		if(ImGui::Button(ICON_FA_FOLDER_OPEN)) {
			_selector.show();
		}
		ImGui::SameLine();

		auto clean_name = [=](auto&& n) { return context()->asset_store().filesystem()->filename(n); };
		core::String name = context()->asset_store().name(_selected).map(clean_name).unwrap_or("No mesh");

		ImGui::SetNextItemWidth(-1);
		ImGui::InputText("", name.data(), name.size(), ImGuiInputTextFlags_ReadOnly);
	}

	_selector.paint(recorder, token);

	if(_selected != AssetId::invalid_id()) {
		ImGui::Text("Vertex data:");
		ImGui::SameLine();
		ImGui::Spacing();
		if(ImGui::Button("Copy to clipboard###vertices")) {
			ImGui::SetClipboardText(_vertices.data());
		}

		ImGui::Text("Triangle data:");
		ImGui::SameLine();
		ImGui::Spacing();
		if(ImGui::Button("Copy to clipboard###triangles")) {
			ImGui::SetClipboardText(_triangles.data());
		}
	}
}

void AssetStringifier::stringify(AssetId id) {
	auto data = context()->asset_store().data(id);
	if(!data) {
		log_msg("Unable to fine asset.", Log::Error);
		clear();
		return;
	}

	core::DebugTimer timer("Stringify mesh");

	MeshData mesh;
	serde2::ReadableArchive ar(*data.unwrap());
	if(!mesh.deserialize(ar)) {
		log_msg("Unable to load mesh.", Log::Error);
		clear();
		return;
	}

	_selected = id;
	_vertices.make_empty();
	_triangles.make_empty();

	int prec = 3;

	auto vertices = core::vector_with_capacity<core::String>(mesh.vertices().size());
	std::transform(mesh.vertices().begin(), mesh.vertices().end(), std::back_inserter(vertices), [=](const Vertex& v) {
			std::array<char, 1024> buffer;
			std::snprintf(buffer.data(), buffer.size(), "{{%.*ff, %.*ff, %.*ff}, {%.*ff, %.*ff, %.*ff}, {%.*ff, %.*ff, %.*ff}, {%.*ff, %.*ff}}",
						  prec, v.position.x(), prec, v.position.y(), prec, v.position.z(),
						  prec, v.normal.x(), prec, v.normal.y(), prec, v.normal.z(),
						  prec, v.tangent.x(), prec, v.tangent.y(), prec, v.tangent.z(),
						  prec, v.uv.x(), prec, v.uv.y());
			return buffer.data();
		});
	fmt_into(_vertices, "%", vertices);
	fmt_into(_triangles, "%", mesh.triangles());

	auto fix_brackets = [](char& c) {
			if(c == '[') {
				c = '{';
			}
			if(c == ']') {
				c = '}';
			}
		};

	for(char& c : _vertices) {
		fix_brackets(c);
	}
	for(char& c : _triangles) {
		fix_brackets(c);
	}
}


void AssetStringifier::clear() {
	_selected = AssetId();
	_vertices.clear();
	_triangles.clear();
}

}
