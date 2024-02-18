/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#include "Renamer.h"

#include <editor/utils/ui.h>

#include <y/utils/log.h>
#include <y/utils/format.h>



#include <algorithm>

namespace editor {

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
        filesystem()->for_each(path, [this, path](const auto& info) {
                const core::String full_name = filesystem()->join(path, info.name);
                if(auto icon = entry_icon(full_name, info.type)) {
                    _entries.emplace_back(Entry {
                        info.name,
                        info.type,
                        std::move(icon.unwrap()),
                        info.file_size
                    });
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

core::Result<UiIcon> FileSystemView::entry_icon(const core::String&, EntryType type) const {
    switch(type) {
        case EntryType::Directory:
            return core::Ok(UiIcon{ICON_FA_FOLDER, imgui::folder_icon_color});

        case EntryType::File:
            return core::Ok(UiIcon{ICON_FA_FILE_ALT, 0xFFFFFFFF});

        case EntryType::Unknown:
            return core::Ok(UiIcon{ICON_FA_QUESTION, 0xFFFFFFFF});

        default:
            return core::Err();
    }
}

UiTexture FileSystemView::entry_thumbmail(const core::String&, EntryType) const {
    return {};
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

    auto make_drop_target = [&](const core::String& drop_path) {
        if(const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(imgui::drag_drop_path_id)) {
            const std::string_view original_name = static_cast<const char*>(payload->Data);
            const FileSystemModel* fs = filesystem();
            const core::String target_name = fs->join(drop_path, fs->filename(original_name));
            if(!fs->rename(original_name, target_name)) {
                log_msg(fmt("Unable to move \"{}\" to \"{}\"", original_name, drop_path), Log::Error);
            }
            refresh_all();
        }
    };

    const bool modify = allow_modify();

    usize new_hovered_index = usize(-1);
    auto post_draw_entry = [&](usize index) {
        const Entry& entry = _entries[index];
        if(modify) {
            if(ImGui::BeginDragDropTarget()) {
                const core::String full_name = entry_full_name(entry);
                make_drop_target(full_name);
                ImGui::EndDragDropTarget();
            } else if(ImGui::BeginDragDropSource()) {
                const core::String full_name = entry_full_name(entry);
                ImGui::SetDragDropPayload(imgui::drag_drop_path_id, full_name.data(), full_name.size() + 1);
                ImGui::EndDragDropSource();
            }
        }

        if(ImGui::IsItemHovered()) {
            new_hovered_index = index;
        }
    };

    auto open_menu_if_needed = [&] {
        const bool menu_openned = process_context_menu();
        if(!menu_openned && _hovered != new_hovered_index) {
            entry_hoverred(entry(new_hovered_index));
            _hovered = new_hovered_index;
        }

    };

    ImGui::BeginChild("##fileentriespanel");

    {
        const ImGuiTableFlags table_flags = ImGuiTableFlags_RowBg;
        if(ImGui::BeginTable("##fileentries", 2, table_flags)) {
            ImGui::TableSetupColumn("##name", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableSetupColumn("##size", ImGuiTableColumnFlags_WidthFixed);

            const auto parent_path = filesystem()->parent_path(path());
            const bool at_root = (!parent_path.is_ok()) || (parent_path.unwrap() == _current_path);
            if(!at_root) {
                imgui::table_begin_next_row();
                if(ImGui::Selectable(ICON_FA_ARROW_LEFT " ..", false, ImGuiSelectableFlags_SpanAllColumns)) {
                    set_path(parent_path.unwrap());
                }

                if(ImGui::BeginDragDropTarget()) {
                    make_drop_target(parent_path.unwrap());
                    ImGui::EndDragDropTarget();
                }
            }

            for(usize i = 0; i != _entries.size(); ++i) {
                imgui::table_begin_next_row();

                const Entry& entry = _entries[i];
                if(imgui::selectable_icon(entry.icon, fmt_c_str("{}##{}", entry.name, i), _hovered == i, ImGuiSelectableFlags_SpanAllColumns)) {
                    entry_clicked(entry);
                    break;
                }

                post_draw_entry(i);

                if(entry.type == EntryType::File) {
                    ImGui::TableNextColumn();
                    ImGui::TextUnformatted(fmt_c_str("{} KB", (entry.file_size + 1023) / 1024, i));
                }
            }

            open_menu_if_needed();

            ImGui::EndTable();
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
            log_msg("Unable to create directory", Log::Error);
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
                log_msg(fmt("Unable to delete {}", full_name), Log::Error);
            }
            refresh_all();
        }
    }
}

}

