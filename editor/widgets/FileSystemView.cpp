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
    set_icon_delegate([](const core::String&, EntryType type) {
        switch(type) {
            case EntryType::Directory:
                return UiIcon{ICON_FA_FOLDER, imgui::folder_icon_color};

            case EntryType::File:
                return UiIcon{ICON_FA_FILE_ALT, 0xFFFFFFFF};

            default:
                return UiIcon{ICON_FA_QUESTION, 0xFFFFFFFF};
        }
    });

    set_filter_delegate([](const core::String&, EntryType) { return true; });

    set_filesystem(fs);
}

const FileSystemModel* FileSystemView::filesystem() const {
    return _filesystem;
}


core::String FileSystemView::root_path() const {
    return _filesystem->current_path().unwrap_or(core::String());
}

void FileSystemView::set_filesystem(const FileSystemModel* fs) {
    _filesystem = fs ? fs : FileSystemModel::local_filesystem();
    set_path(root_path());
    _cached_nodes.clear();
    _need_update = true;
}

void FileSystemView::refresh() {
    _need_update = true;
}

void FileSystemView::set_allow_modify(bool modify) {
    _allow_modify = modify;
}

void FileSystemView::set_split_mode_enabled(bool split) {
    _split_mode = split;
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
        _need_update = true;
    } else if(parent) {
        set_path(parent.unwrap());
    }
}

const core::String& FileSystemView::path() const {
    return _current_path;
}

const FileSystemView::Entry* FileSystemView::entry(usize index) const {
    if(index < _entries.size()) {
        return &_entries[index];
    }
    return nullptr;
}

void FileSystemView::entry_clicked(const Entry& entry) {
    if(!_clicked_delegate(entry.full_name, entry.type)) {
        if(entry.type == EntryType::Directory) {
            set_path(entry.full_name);
        }
    }
}

void FileSystemView::update_nodes(std::string_view path, core::Vector<TreeNode>& nodes) {
    for(TreeNode& node : nodes) {
        node.to_delete = true;
    }

    filesystem()->for_each(path, [&](const auto& info) {
        if(info.type != EntryType::Directory) {
            return;
        }

        core::String full_name = filesystem()->join(path, info.name);
        const auto it = std::find_if(nodes.begin(), nodes.end(), [&](const TreeNode& node) { return node.name == info.name; });
        if(it == nodes.end()) {
            TreeNode& node = nodes.emplace_back();
            node.name = info.name;
            node.full_name = std::move(full_name);
        } else {
            TreeNode& node = *it;
            node.to_delete = false;
            if(!node.expanded) {
                node.children.make_empty();
            } else {
                update_nodes(full_name, node.children);
            }
        }
    }).ignore();

    for(usize i = 0; i < nodes.size(); ++i) {
        if(nodes[i].to_delete) {
            nodes.erase_unordered(nodes.begin() + i);
            --i;
        }
    }
}

void FileSystemView::expand_node_path(std::string_view path, core::Vector<TreeNode>& nodes) {
    for(TreeNode& node : nodes) {
        if(node.full_name.size() < path.size() && path.starts_with(node.full_name)) {
            node.expanded = true;
            update_nodes(node.full_name, node.children);
            expand_node_path(path, node.children);
        }
    }
}

void FileSystemView::update() {
    y_profile();

    core::String hoverred_name;
    if(_selected_index < _entries.size()) {
        hoverred_name = _entries[_selected_index].name;
    }

    _selected_index = usize(-1);
    _entries.clear();

    const std::string_view path = _current_path;
    if(filesystem()->exists(path).unwrap_or(false)) {
        filesystem()->for_each(path, [this, path](const auto& info) {
            core::String full_name = filesystem()->join(path, info.name);
            if(_filter_delegate(full_name, info.type)) {
                const UiIcon icon = _icon_delegate(full_name, info.type);
                _entries.emplace_back(Entry {
                    std::move(full_name),
                    info.name,
                    info.type,
                    icon,
                });
            }
        }).ignore();
        std::sort(_entries.begin(), _entries.end());
    } else {
        if(const auto p = filesystem()->parent_path(path)) {
            set_path(p.unwrap());
        }
    }

    if(_split_mode) {
        update_nodes(root_path(), _cached_nodes);
        expand_node_path(_current_path, _cached_nodes);
    }

    for(usize i = 0; i != _entries.size(); ++i) {
        if(_entries[i].name == hoverred_name) {
            _selected_index = i;
            break;
        }
    }

    _update_chrono.reset();
    _need_update = false;

    _on_update();
}


void FileSystemView::on_gui() {
    y_profile();

    if(_need_update || _update_chrono.elapsed() > update_duration) {
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

    auto post_draw_entry = [&](usize index) {
        const Entry& entry = _entries[index];
        if(_allow_modify) {
            if(ImGui::BeginDragDropTarget()) {
                make_drop_target(entry.full_name);
                ImGui::EndDragDropTarget();
            } else if(ImGui::BeginDragDropSource()) {
                ImGui::SetDragDropPayload(imgui::drag_drop_path_id, entry.full_name.data(), entry.full_name.size() + 1);
                ImGui::EndDragDropSource();
            }
        }
    };

    auto open_menu_if_needed = [&] {
        if(imgui::should_open_context_menu()) {
            ImGui::OpenPopup("##contextmenu");
        }

        if(ImGui::BeginPopup("##contextmenu")) {
            draw_context_menu();
            ImGui::EndPopup();
        }
    };


    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    y_defer(ImGui::PopStyleColor());


    if(_split_mode) {
        if(ImGui::BeginChild("##treepanel", ImVec2(200.0f, -1.0))) {
            const bool root_open = ImGui::TreeNodeEx(ICON_FA_DATABASE " root", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DefaultOpen);

            if(!ImGui::IsItemToggledOpen() && ImGui::IsItemActivated()) {
                set_path(root_path());
            }

            if(root_open) {
                for(TreeNode& child : _cached_nodes) {
                    draw_node(child);
                }
                ImGui::TreePop();
            }
        }
        ImGui::EndChild();

        ImGui::SameLine();

        if(ImGui::BeginChild("##fileentriespanel")) {
            const float icon_size = 60.0f;
            const usize entry_per_row = std::max(1_uu, usize(ImGui::GetContentRegionAvail().x / icon_size));


            ImGuiListClipper clipper;
            clipper.Begin(int(_entries.size() / entry_per_row) + 1);

            bool done = false;
            while(!done && clipper.Step()) {
                for(usize row = clipper.DisplayStart; !done && row < clipper.DisplayEnd; ++row) {
                    for(usize i = 0; i != entry_per_row; ++i) {
                        const usize index = row * entry_per_row + i;
                        if(index >= _entries.size()) {
                            done = true;
                            break;
                        }

                        const Entry& entry = _entries[index];
                        const bool selected = (_selected_index == index);

                        bool clicked = false;
                        bool double_clicked = ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left);
                        if(UiTexture preview = _preview_delegate(entry.full_name, entry.type)) {
                            clicked = imgui::icon_button(preview, entry.name.data(), selected, icon_size, ImGuiSelectableFlags_AllowDoubleClick);
                        } else {
                            clicked = imgui::icon_button(entry.icon, entry.name.data(), selected, icon_size, ImGuiSelectableFlags_AllowDoubleClick);
                        }
                        double_clicked &= clicked;
                        const bool is_right_clicked = ImGui::IsItemHovered() && ImGui::IsMouseClicked(ImGuiMouseButton_Right);

                        if((clicked || is_right_clicked) && !double_clicked) {
                            _selected_index = index;
                        }

                        if(i + 1 < entry_per_row) {
                            ImGui::SameLine();
                        }

                        if(double_clicked) {
                            entry_clicked(entry);
                            if(_need_update) {
                                done = true;
                                break;
                            }
                        }
                    }
                }
            }

            open_menu_if_needed();
        }
        ImGui::EndChild();

    } else {
        if(ImGui::BeginChild("##fileentriespanel")) {
            const ImGuiTableFlags table_flags = ImGuiTableFlags_RowBg;
            if(ImGui::BeginTable("##fileentries", 2, table_flags)) {
                ImGui::TableSetupColumn("##name", ImGuiTableColumnFlags_WidthStretch);

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
                    if(imgui::selectable_icon(entry.icon, fmt_c_str("{}##{}", entry.name, i), _selected_index == i, ImGuiSelectableFlags_SpanAllColumns)) {
                        entry_clicked(entry);
                        if(_need_update) {
                            break;
                        }
                    }

                    post_draw_entry(i);
                }

                open_menu_if_needed();

                ImGui::EndTable();
            }
        }
        ImGui::EndChild();
    }
}

void FileSystemView::draw_node(TreeNode& node) {
    const bool selected = node.expanded ? _current_path == node.full_name : _current_path.starts_with(node.full_name);
    const int flags = (node.expanded ? ImGuiTreeNodeFlags_DefaultOpen : 0) | (selected ? ImGuiTreeNodeFlags_Selected : 0);

    const bool opened = ImGui::TreeNodeEx(fmt_c_str("{} {}###{}", node.expanded ? ICON_FA_FOLDER_OPEN : ICON_FA_FOLDER, node.name, node.full_name), ImGuiTreeNodeFlags_OpenOnArrow | flags);
    if(node.expanded != opened) {
        if((node.expanded = opened)) {
            _need_update = true;
        }
    }

    if(!ImGui::IsItemToggledOpen() && ImGui::IsItemActivated()) {
        set_path(node.full_name);
    }

    if(opened) {
        for(TreeNode& child : node.children) {
            draw_node(child);
        }
        ImGui::TreePop();
    }
}


void FileSystemView::draw_context_menu() {
    if(ImGui::Selectable("New folder")) {
        if(!filesystem()->create_directory(filesystem()->join(path(), "new folder"))) {
            log_msg("Unable to create directory", Log::Error);
        }
        refresh_all();
    }

    if(_allow_modify && _selected_index < _entries.size()) {
        ImGui::Separator();

        const auto& entry = _entries[_selected_index];
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

