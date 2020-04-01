/*******************************
Copyright (c) 2016-2020 Gr�goire Angerand

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

#include "FilesystemView.h"

#include <editor/context/EditorContext.h>
#include <editor/utils/ui.h>

#include <y/utils/sort.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

#include <imgui/yave_imgui.h>

namespace editor {

FileSystemView::FileSystemView(const FileSystemModel* fs) : Widget("File browser") {
	set_filesystem(fs);
}


void FileSystemView::set_filesystem(const FileSystemModel* model) {
	_filesystem = model ? model : FileSystemModel::local_filesystem();
	set_path(_filesystem->current_path().unwrap_or(core::String()));
}

const FileSystemModel* FileSystemView::filesystem() const {
	return _filesystem;
}

void FileSystemView::refresh() {
	_refresh = true;
}

void FileSystemView::set_allow_modify_filesystem(bool allow) {
	_allow_modify = allow;
}


void FileSystemView::set_path(std::string_view path) {
	y_profile();

	if(!filesystem()->exists(path).unwrap_or(false)) {
		if(const auto parent = filesystem()->parent_path(path)) {
			set_path(parent.unwrap());
		} else {
			// reset path
			set_filesystem(_filesystem);
		}
		return;
	}

	if(filesystem()->is_directory(path).unwrap_or(false)) {
		if(auto abs = filesystem()->absolute(path)) {
			_current_path = std::move(abs.unwrap());
		} else {
			log_msg("Unable to create absolute path.", Log::Warning);
			_current_path = path;
		}
		update();
		path_changed();
	}

}

const core::String& FileSystemView::path() const {
	return _current_path;
}

usize FileSystemView::hoverred_index() const {
	return _hovered;
}

const FileSystemView::Entry* FileSystemView::entry(usize index) const {
	if(index < _entries.size()) {
		return &_entries[index];
	}
	return nullptr;
}





void FileSystemView::update() {
	y_profile();

	core::String hoverred_name;
	if(_hovered < _entries.size()) {
		hoverred_name = _entries[_hovered].name;
	}

	_hovered = usize(-1);
	_entries.clear();

	const std::string_view path = _current_path;
	if(filesystem()->exists(path).unwrap_or(false)) {
		filesystem()->for_each(path, [this, path](const auto& name) {
				const bool is_dir = filesystem()->is_directory(filesystem()->join(path, name)).unwrap_or(false);
				const EntryType type = is_dir ? EntryType::Directory : EntryType::File;
				if(auto icon = entry_icon(name, type)) {
					_entries.emplace_back(Entry{name, type, std::move(icon.unwrap())});
				}
			}).ignore();
		y::sort(_entries.begin(), _entries.end(), [](const auto& a, const auto& b) { return a.type < b.type; });
	} else {
		if(const auto p =filesystem()->parent_path(path)) {
			set_path(p.unwrap());
		}
	}

	for(usize i = 0; i != _entries.size(); ++i) {
		if(_entries[i].name == hoverred_name) {
			_hovered = i;
			break;
		}
	}

	_update_chrono.reset();
	_refresh = false;
}



core::Result<core::String> FileSystemView::entry_icon(const core::String&, EntryType type) const {
	return type == EntryType::Directory
		? core::Ok(core::String(ICON_FA_FOLDER))
		: core::Ok(core::String(ICON_FA_FILE_ALT));
}

void FileSystemView::entry_clicked(const Entry& entry) {
	if(entry.type == EntryType::Directory) {
		set_path(filesystem()->join(path(), entry.name));
	}
}

void FileSystemView::paint_ui(CmdBufferRecorder&, const FrameToken&) {
	y_profile();

	ImGui::BeginChild("###fileentries", ImVec2(), true);

	usize hovered = usize(-1);
	{
		if(ImGui::Selectable(ICON_FA_ARROW_LEFT " ..")) {
			if(const auto p = filesystem()->parent_path(path())) {
				set_path(p.unwrap());
			}
		}

		for(usize i = 0; i != _entries.size(); ++i) {
			if(ImGui::Selectable(fmt_c_str("% %", _entries[i].icon, _entries[i].name), _hovered == i)) {
				entry_clicked(_entries[i]);
				break; // break because we might update inside entry_clicked
			}

			if(ImGui::IsItemHovered()) {
				hovered = i;
			}
		}
	}

	const bool menu_openned = process_context_menu();
	if(!menu_openned && _hovered != hovered) {
		entry_hoverred(entry(hovered));
		_hovered = hovered;
	}
	if(_refresh || _update_chrono.elapsed() > update_duration) {
		update();
	}

	ImGui::EndChild();
}

bool FileSystemView::process_context_menu() {
	bool menu_openned = false;
	if(_allow_modify) {
		if(imgui::should_open_context_menu()) {
			ImGui::OpenPopup("###contextmenu");
		}

		if(ImGui::BeginPopup("###contextmenu")) {
			menu_openned = true;
			draw_context_menu();
			ImGui::EndPopup();
		}
	}
	return menu_openned;
}

void FileSystemView::draw_context_menu() {
	if(_hovered < _entries.size()) {
		const auto& entry = _entries[_hovered];
		if(ImGui::MenuItem("Delete")) {
			if(const core::String full_name = filesystem()->join(_current_path, entry.name)) {
				if(!filesystem()->remove(full_name)) {
					log_msg(fmt("Unable to delete %", full_name), Log::Error);
				}
			} else {
				log_msg(fmt("Unable to delete %", entry.name), Log::Error);
			}
			refresh();
		}
	}

	if(ImGui::Selectable("New folder")) {
		if(!filesystem()->create_directory(filesystem()->join(path(), "new folder"))) {
			log_msg("Unable to create directory.", Log::Error);
		}
		refresh();
	}
}

}
