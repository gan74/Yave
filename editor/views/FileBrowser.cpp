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

FileBrowser::FileBrowser() :
		Frame("File browser"),
		_current(stdfs::current_path()),
		_callback(Nothing()) {

	_input_path.set_min_capacity(512);
	_input_path = _current.string();
}

void FileBrowser::paint_ui(CmdBufferRecorder<>&, const FrameToken&) {
	if(ImGui::InputText("Path", _input_path, _input_path.capacity(), ImGuiInputTextFlags_EnterReturnsTrue)) {
		if(stdfs::exists(_input_path.data())) {
			_current = stdfs::path(_input_path.data());
		}
	}
	ImGui::SameLine();
	if(ImGui::Button("Cancel")) {
		_visible = false;
	}


	if(ImGui::Button("..")) {
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
	}

}

}
