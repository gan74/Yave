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

#include "FileBrowser.h"

#include <imgui/imgui.h>


namespace editor {

template<usize N>
static void to_buffer(std::array<char, N>& buffer, std::string_view str) {
	usize len = std::min(buffer.size() - 1, str.size());
	std::copy_n(str.begin(), len, buffer.begin());
	buffer[len] = 0;
}

FileBrowser::FileBrowser(NotOwner<FileSystemModel*> model) :
		Widget("File browser"),
		_name_buffer({0}),
		_callback(Nothing()) {

	to_buffer(_path_buffer, "");
	to_buffer(_name_buffer, "");

	set_filesystem(model);
}

void FileBrowser::done(const core::String& filename) {
	_callback(filename);
	_visible = false;
}

void FileBrowser::cancel() {
	_visible = false;
}

void FileBrowser::set_filesystem(NotOwner<FileSystemModel*> model) {
	_model = model ? model : FileSystemModel::local_filesystem();
	set_path(_model->current_path());
}

void FileBrowser::set_path(std::string_view path) {
	_entries.clear();
	if(!_model->exists(path)) {
		return;
	}

	_model->for_each(path, [this](const auto& name) {
		_entries.push_back(name);
	});
	if(_model->is_directory(path)) {
		to_buffer(_path_buffer, path);
	} else {
		done(path);
		set_path(_model->parent_path(path));
	}
}

core::String FileBrowser::full_path() const {
	std::string_view name(_name_buffer.begin(), std::strlen(_name_buffer.begin()));
	return _model->join(path(), name);
}

std::string_view FileBrowser::path() const {
	return std::string_view(_path_buffer.begin(), std::strlen(_path_buffer.begin()));
}

void FileBrowser::paint_ui(CmdBufferRecorder<>&, const FrameToken&) {
	{
		if(ImGui::InputText("###path", _path_buffer.begin(), _path_buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
			set_path(full_path());
		}
		ImGui::SameLine();
		if(ImGui::Button("Ok")) {
			done(full_path());
		}
	}

	{
		if(ImGui::InputText("###filename", _name_buffer.begin(), _name_buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
			set_path(full_path());
		}
		ImGui::SameLine();
		if(ImGui::Button("Cancel")) {
			cancel();
		}
	}

	ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyle().Colors[ImGuiCol_ModalWindowDarkening]);
	ImGui::BeginChild("###fileentries");
	{
		if(ImGui::Selectable("..", _selection == 0)) {
			set_path(_model->parent_path(path()));
		}

		for(usize i = 0; i != _entries.size(); ++i) {
			const auto& name = _entries[i];
			if(ImGui::Selectable(name.data(), _selection == i)) {
				set_path(_model->join(path(), name));
				break;
			}
		}
	}
	ImGui::EndChild();
	ImGui::PopStyleColor();
}

}
