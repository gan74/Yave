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

#include <editor/context/EditorContext.h>

#include <imgui/imgui.h>

namespace editor {

AssetRenamer::AssetRenamer(ContextPtr ctx, std::string_view name) :
		Widget("Rename", ImGuiWindowFlags_AlwaysAutoResize),
		ContextLinked(ctx),
		_name(name) {

	usize size = std::min(_new_name.size(), name.size() + 1);
	std::copy_n(_name.begin(), size, _new_name.begin());

}

void AssetRenamer::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	ImGui::Text("Rename: \"%s\"", _name.data());
	ImGui::InputText("", _new_name.data(), _new_name.size());

	if(ImGui::Button("Ok")) {
		try {
			context()->asset_store().rename(_name, _new_name.data());
			context()->ui().for_each<ResourceBrowser>([](ResourceBrowser* b) { b->refresh(); });
			close();
		} catch(std::exception& e) {
			context()->ui().ok("Error", e.what());
		}
	}

	ImGui::SameLine();
	if(ImGui::Button("Cancel")) {
		close();
	}
}

}
