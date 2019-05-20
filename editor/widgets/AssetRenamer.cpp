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

#include "AssetRenamer.h"
#include "ResourceBrowser.h"

#include <yave/utils/FileSystemModel.h>

#include <editor/context/EditorContext.h>

#include <imgui/yave_imgui.h>

namespace editor {

/*static std::string_view decompose_path(std::string_view full_name, std::string_view name) {
	y_debug_assert(full_name.size() >= name.size());
	y_debug_assert(full_name.substr(full_name.size() - name.size()) == name);
	return full_name.substr(0, full_name.size() - name.size());
}*/


AssetRenamer::AssetRenamer(ContextPtr ctx, std::string_view full_name) :
		Widget("Rename", ImGuiWindowFlags_AlwaysAutoResize),
		ContextLinked(ctx),
		_full_name(full_name),
		_name(filesystem()->filename(_full_name)) {

	usize size = std::min(_new_name.size(), _name.size() + 1);
	std::copy_n(_name.begin(), size, _new_name.begin());
}

void AssetRenamer::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	ImGui::Text("Rename: \"%s\"", _name.data());
	ImGui::InputText("", _new_name.data(), _new_name.size());

	if(ImGui::Button("Ok")) {
		auto path = filesystem()->parent_path(_full_name);
		auto full_new_name = path.map([=](auto&& p) { return filesystem()->join(p, _new_name.data()); });
		if(full_new_name && context()->asset_store().rename(_full_name, full_new_name.unwrap())) {
			context()->ui().refresh_all();
			close();
		} else {
			log_msg("Unable to rename asset.", Log::Error);
		}
	}

	ImGui::SameLine();
	if(ImGui::Button("Cancel")) {
		close();
	}
}

const FileSystemModel* AssetRenamer::filesystem() const {
	return context()->asset_store().filesystem();
}


}
