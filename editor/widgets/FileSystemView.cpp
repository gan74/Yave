/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include "FileSystemView.h"
#include "FileRenamer.h"

#include <editor/utils/ui.h>

#include <yave/utils/FileSystemModel.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <external/imgui/yave_imgui.h>

#include <algorithm>

namespace editor {

static constexpr const char* drag_drop_id = "YAVE_DRAG_DROP_PATH";

FileSystemView::FileSystemView(const FileSystemModel* fs, std::string_view name) : Widget(name) {
    set_filesystem(fs);
}

void FileSystemView::set_filesystem(const FileSystemModel* model) {
    _filesystem = model ? model : FileSystemModel::local_filesystem();
    set_path(_filesystem->current_path().unwrap_or(core::String()));
    _refresh = true;
}

const FileSystemModel* FileSystemView::filesystem() const {
    return _filesystem;
}

void FileSystemView::refresh() {
    _refresh = true;
}

void FileSystemView::set_path(const core::String& path) {
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

    core::String absolute = filesystem()->absolute(path).unwrap_or(path);
    const auto parent = filesystem()->parent_path(absolute);
    if(filesystem()->is_directory(absolute).unwrap_or(false)) {
        _current_path = std::move(absolute);

        _at_root = parent.is_error() || (parent.unwrap() == _current_path);

        update();
        path_changed();
    } else if(parent) {
        set_path(parent.unwrap());
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


core::String FileSystemView::entry_full_name(const Entry& entry) const {
    return filesystem()->join(path(), entry.name);
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
                const core::String full_name = filesystem()->join(path, name);
                const bool is_dir = filesystem()->is_directory(full_name).unwrap_or(false);
                const EntryType type = is_dir ? EntryType::Directory : EntryType::File;
                if(auto icon = entry_icon(full_name, type)) {
                    _entries.emplace_back(Entry{name, type, std::move(icon.unwrap())});
                }
            }).ignore();
        std::sort(_entries.begin(), _entries.end());
    } else {
        if(const auto p = filesystem()->parent_path(path)) {
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
        set_path(entry_full_name(entry));
    }
}

bool FileSystemView::allow_modify() const {
    return true;
}

void FileSystemView::on_gui() {
    y_profile();

    if(_refresh || _update_chrono.elapsed() > update_duration) {
        update();
    }

    const bool modify = allow_modify();
    ImGui::BeginChild("##fileentries", ImVec2(), true);
    {
        imgui::alternating_rows_background();
        usize hovered = usize(-1);
        {
            const auto parent_path = filesystem()->parent_path(path());
            if(!_at_root) {
                if(ImGui::Selectable(ICON_FA_ARROW_LEFT " ..")) {
                    if(parent_path) {
                        set_path(parent_path.unwrap());
                    }
                }
                if(modify && parent_path) {
                    make_drop_target(parent_path.unwrap());
                }
            }

            for(usize i = 0; i != _entries.size(); ++i) {
                if(ImGui::Selectable(fmt_c_str("% %", _entries[i].icon, _entries[i].name), _hovered == i)) {
                    entry_clicked(_entries[i]);
                    break; // break because we might update inside entry_clicked
                }

                if(modify) {
                    // Is this slow?
                    const core::String full_name = entry_full_name(_entries[i]);
                    make_drop_target(full_name);
                    if(ImGui::BeginDragDropSource()) {
                        ImGui::SetDragDropPayload(drag_drop_id, full_name.data(), full_name.size() + 1);
                        ImGui::EndDragDropSource();
                    }
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
    }
    ImGui::EndChild();
}

bool FileSystemView::process_context_menu() {
    bool menu_openned = false;
    if(imgui::should_open_context_menu()) {
        ImGui::OpenPopup("##contextmenu");
    }

    if(ImGui::BeginPopup("##contextmenu")) {
        menu_openned = true;
        draw_context_menu();
        ImGui::EndPopup();
    }
    return menu_openned;
}

void FileSystemView::draw_context_menu() {
    if(ImGui::Selectable("New folder")) {
        if(!filesystem()->create_directory(filesystem()->join(path(), "new folder"))) {
            log_msg("Unable to create directory.", Log::Error);
        }
        refresh_all();
    }

    const bool modify = allow_modify();
    if(modify && _hovered < _entries.size()) {
        ImGui::Separator();

        const auto& entry = _entries[_hovered];
        const core::String full_name = filesystem()->join(_current_path, entry.name);

        if(ImGui::MenuItem("Rename")) {
            add_detached_widget<FileRenamer>(filesystem(), full_name);
        }

        if(ImGui::MenuItem("Delete")) {
            if(!filesystem()->remove(full_name)) {
                log_msg(fmt("Unable to delete %", full_name), Log::Error);
            }
            refresh_all();
        }
    }
}

void FileSystemView::make_drop_target(std::string_view drop_path) {
    if(ImGui::BeginDragDropTarget()) {
        if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(drag_drop_id)) {
            const std::string_view original_name = reinterpret_cast<const char*>(payload->Data);
            const FileSystemModel* fs = filesystem();
            const core::String target_name = fs->join(drop_path, fs->filename(original_name));
            if(!fs->rename(original_name, target_name)) {
                log_msg(fmt("Unable to move \"%\" to \"%\".", original_name, drop_path), Log::Error);
            }
            refresh_all();
        }
        ImGui::EndDragDropTarget();
    }
}

}

