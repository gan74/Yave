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


namespace stdfs = std::experimental::filesystem;

namespace editor {

template<usize N>
static void to_buffer(std::array<char, N>& buffer, const stdfs::path& p) {
	auto n = p.string();
	std::copy_n(n.begin(), std::min(buffer.size() - 1, n.size()), buffer.begin());
	buffer[n.size()] = 0;
}

FileBrowser::FileBrowser() :
		Frame("File browser"),
		_name_buffer({0}),
		_callback(Nothing()) {

	set_path(stdfs::current_path());
}

void FileBrowser::done() {
	input_path();
	_callback(_current.string());
	_visible = false;
}

void FileBrowser::cancel() {
	_visible = false;
}

void FileBrowser::set_path(const std::experimental::filesystem::path& path) {
	_current = path;
	if(stdfs::is_directory(_current)) {
		to_buffer(_path_buffer, _current);
	} else {
		to_buffer(_path_buffer, _current.parent_path());
		to_buffer(_name_buffer, _current.filename());
	}
}

void FileBrowser::input_path() {
	stdfs::path path(_path_buffer.begin());
	path /= stdfs::path(_name_buffer.begin());
	set_path(path);
}

void FileBrowser::paint_ui(CmdBufferRecorder<>&, const FrameToken&) {
	{
		if(ImGui::InputText("###path", _path_buffer.begin(), _path_buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
			input_path();
		}
		ImGui::SameLine();
		if(ImGui::Button("Ok")) {
			done();
		}
	}

	{
		if(ImGui::InputText("###filename", _name_buffer.begin(), _name_buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
			input_path();
		}
		ImGui::SameLine();
		if(ImGui::Button("Cancel")) {
			cancel();
		}
	}

	int count = 0;
	stdfs::path dir = stdfs::is_directory(_current) ? _current : _current.parent_path();

	if(ImGui::Selectable("..", _selection == count++)) {
		set_path(_current.parent_path());
	}

	for(auto& e : stdfs::directory_iterator(dir)) {

		stdfs::path sub = e.path();
		auto name = sub.filename().string();

		if(ImGui::Selectable(name.c_str(), _selection == count++)) {
			set_path(sub);
			break;
		}
	}


	/*if(ImGui::Button("..")) {
		_current = _current.parent_path();
	}

	for(auto& e : stdfs::directory_iterator(_current)) {

		stdfs::path sub = e.path();
		auto name = sub.filename().string();

		if(stdfs::is_regular_file(sub)) {
			if(sub.filename().extension() == ".ys") {
				if(ImGui::Button(name.c_str())) {
					_callback(name);
					_visible = false;
				}
			}
			ImGui::Text(name.c_str());
		} else if(stdfs::is_directory(sub)) {
			if(ImGui::Button(name.c_str())) {
				_current = sub;
			}
		} else {
			ImGui::Text(name.c_str());
		}
	}*/

}

}
