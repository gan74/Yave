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

#include "ResourceBrowser.h"

#include <editor/context/EditorContext.h>

#include <imgui/imgui.h>

namespace editor {

ResourceBrowser::ResourceBrowser(ContextPtr ctx) :
		Widget("Resource browser"),
		ContextLinked(ctx),
		_root("", filesystem()->current_path()) {

	set_current(&_root);
}


void ResourceBrowser::set_current(DirNode* current) {
	if(!current) {
		set_current(&_root);
		return;
	}

	_current = current;

	_current->children.clear();
	_current->files.clear();

	const auto* fs = filesystem();
	auto current_dir = fs->join(_current->path, _current->name);
	log_msg(_current->path + " " + _current->name + " = " + current_dir);
	fs->for_each(current_dir, [&](const auto& name) {
			auto full_name = fs->join(current_dir, name);
			if(fs->is_directory(full_name)) {
				_current->children << DirNode(name, current_dir, _current);
			} else {
				_current->files << name;
			}
		});
}

void ResourceBrowser::paint_ui(CmdBufferRecorder<>& recorder, const FrameToken& token) {
	unused(recorder, token);

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_ModalWindowDarkening]);
	ImGui::BeginChild("###resources");
	{
		if(ImGui::IsMouseReleased(1)) {
			ImGui::OpenPopup("###resourcescontext");
		}

		if(ImGui::BeginPopupContextItem("###resourcescontext")) {
			ImGui::Selectable("Import");

			ImGui::EndPopup();
		}

		if(_current->parent && ImGui::Selectable("..")) {
			set_current(_current->parent);
		}

		auto curr = _current;
		for(DirNode& n : curr->children) {
			if(ImGui::Selectable(n.name.data())) {
				set_current(&n);
			}
		}

		for(const auto& name : curr->files) {
			if(ImGui::Selectable(name.data())) {
			}
		}
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();

}

const FileSystemModel* ResourceBrowser::filesystem() const {
	return context()->loader().asset_store().filesystem();
}

}
