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

#include "FileBrowser.h"

#include <imgui/yave_imgui.h>


namespace editor {

template<usize N>
static void to_buffer(std::array<char, N>& buffer, std::string_view str) {
	usize len = std::min(buffer.size() - 1, str.size());
	std::copy_n(str.begin(), len, buffer.begin());
	buffer[len] = 0;
}

FileBrowser::FileBrowser(const FileSystemModel* filesystem) :
		Widget("File browser"),
		_name_buffer({0}) {

	to_buffer(_path_buffer, "");
	to_buffer(_name_buffer, "");

	set_filesystem(filesystem);
}


void FileBrowser::set_filesystem(const FileSystemModel* model) {
	_filesystem = model ? model : FileSystemModel::local_filesystem();
	set_path(_filesystem->current_path().unwrap_or(""));
}

void FileBrowser::set_path(std::string_view path) {
	if(!_filesystem->exists(path).unwrap_or(false)) {
		set_path(_last_path);
		return;
	}

	_entries.clear();

	if(_filesystem->is_directory(path).unwrap_or(false)) {
		_filesystem->for_each(path, [this, path](const auto& name) {
				EntryType type = EntryType::Directory;
				if(!_filesystem->is_directory(_filesystem->join(path, name)).unwrap_or(false)) {
					auto ext = _filesystem->extention(name);
					type = std::binary_search(_extensions.begin(), _extensions.end(), ext)
						? EntryType::Supported : EntryType::Unsupported;
				}
				_entries.push_back(std::make_pair(name, type));
			}).ignore();

		y::sort(_entries.begin(), _entries.end(), [](const auto& a, const auto& b) { return a.second < b.second; });

		to_buffer(_path_buffer, path);
		_last_path = path;
	} else {
		done(path);
		if(auto p =_filesystem->parent_path(path)) {
			set_path(p.unwrap());
		}
	}
}

void FileBrowser::set_extension_filter(std::string_view exts) {
	_extensions = core::Vector<core::String>(1, core::String());
	for(char c : exts) {
		if(c == ';') {
			_extensions.emplace_back();
		} else if(c != '*') {
			_extensions.last().push_back(c);
		}
	}
	sort(_extensions.begin(), _extensions.end());
	set_path(_path_buffer.begin());
}

void FileBrowser::done(const core::String& filename) {
	_visible = !_callbacks.selected(filename);
}

void FileBrowser::cancel() {
	_visible = !_callbacks.canceled();
}

core::String FileBrowser::full_path() const {
	std::string_view name(_name_buffer.begin(), std::strlen(_name_buffer.begin()));
	return _filesystem->join(path(), name);
}

std::string_view FileBrowser::path() const {
	return std::string_view(_path_buffer.begin(), std::strlen(_path_buffer.begin()));
}


void FileBrowser::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	static constexpr isize button_width = 75;

	{
		ImGui::SetNextItemWidth(-button_width);
		if(ImGui::InputText("###path", _path_buffer.begin(), _path_buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
			set_path(full_path());
		}
		ImGui::SameLine();
		if(ImGui::Button("Ok")) {
			done(full_path());
		}
	}

	{
		ImGui::SetNextItemWidth(-button_width);
		if(ImGui::InputText("###filename", _name_buffer.begin(), _name_buffer.size(), ImGuiInputTextFlags_EnterReturnsTrue)) {
			set_path(full_path());
		}

		ImGui::SameLine();
		if(ImGui::Button("Cancel")) {
			cancel();
		}
	}

	{
		ImGui::BeginChild("###fileentries");
		{
			if(ImGui::Selectable(ICON_FA_ARROW_LEFT " ..")) {
				if(auto p = _filesystem->parent_path(path())) {
					set_path(p.unwrap());
				}
			}

			const char* icons[] = {ICON_FA_FOLDER, ICON_FA_FILE_ALT, ICON_FA_QUESTION};

			for(usize i = 0; i != _entries.size(); ++i) {
				const auto& name = _entries[i].first;
				if(ImGui::Selectable(fmt("% %", icons[usize(_entries[i].second)], name).data())) {
					set_path(_filesystem->join(path(), name));
					break;
				}
			}
		}
		ImGui::EndChild();
	}
}

}
