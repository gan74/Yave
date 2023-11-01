/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

#include "GltfImporter.h"
#include "ImageImporter.h"

#include <editor/ThumbmailRenderer.h>

#include <editor/utils/assets.h>
#include <editor/utils/ui.h>

#include <yave/assets/AssetStore.h>
#include <yave/utils/FileSystemModel.h>
#include <yave/material/MaterialData.h>

#include <y/io2/Buffer.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <y/serde3/archives.h>



namespace editor {

editor_action("Import glTF", add_detached_widget<GltfImporter>)
editor_action("Import image", add_detached_widget<ImageImporter>)


ResourceBrowser::ResourceBrowser() : ResourceBrowser(ICON_FA_FOLDER_OPEN " Resource Browser") {
}

ResourceBrowser::ResourceBrowser(std::string_view title) : FileSystemView(asset_store().filesystem(), title) {
    set_flags(ImGuiWindowFlags_NoScrollbar);
    ResourceBrowser::path_changed();
}

AssetId ResourceBrowser::asset_id(std::string_view name) const {
    return asset_store().id(name).unwrap_or(AssetId());
}

AssetType ResourceBrowser::read_file_type(AssetId id) const {
    return asset_store().asset_type(id).unwrap_or(AssetType::Unknown);
}

bool ResourceBrowser::is_searching() const {
    return !!_search_results;
}

void ResourceBrowser::draw_import_menu() {
    if(ImGui::Selectable("Import glTF")) {
        add_detached_widget<GltfImporter>(path());
    }
    if(ImGui::Selectable("Import image")) {
        add_detached_widget<ImageImporter>(path());
    }
}

void ResourceBrowser::draw_context_menu() {
    FileSystemView::draw_context_menu();

    ImGui::Separator();

    draw_import_menu();
}

void ResourceBrowser::draw_top_bar() {
    ImGui::PushID("##pathbar");

    if(ImGui::Button(ICON_FA_PLUS " Import")) {
        ImGui::OpenPopup("##importmenu");
    }

    if(ImGui::BeginPopup("##importmenu")) {
        draw_import_menu();
        ImGui::EndPopup();
    }

    {
        ImGui::PushStyleColor(ImGuiCol_Button, 0);
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);

        const usize path_size = _path_pieces.size();
        for(usize i = 0; i != path_size; ++i) {
            const usize index = path_size - 1 - i;
            const auto& piece = _path_pieces[index];

            auto full_path = [&] {
                core::String p = piece;
                for(usize j = index + 1; j < path_size; ++j) {
                    p = filesystem()->join(_path_pieces[j], p);
                }
                return p;
            };

            auto full_name = [&](std::string_view file) -> core::String {
                return filesystem()->join(full_path(), file);
            };

            ImGui::SameLine();
            if(ImGui::Button(fmt_c_str("{}##{}", piece.is_empty() ? "/" : piece, i))) {
                _set_path_deferred = full_path();
            }

            ImGui::SameLine();
            const core::String menu_name = fmt("##jumpmenu_{}", i);
            if(ImGui::Button(fmt_c_str(ICON_FA_ANGLE_RIGHT "##{}", i))) {
                _jump_menu.make_empty();
                filesystem()->for_each(full_path(), [&, this](const auto& info) {
                        if(info.type == EntryType::Directory) {
                            _jump_menu << info.name;
                        }
                    }).ignore();

                ImGui::OpenPopup(menu_name.data());
            }

            if(ImGui::BeginPopup(menu_name.data())) {
                for(const core::String& dir : _jump_menu) {
                    if(ImGui::Selectable(fmt_c_str(ICON_FA_FOLDER " {}", dir))) {
                        _set_path_deferred = full_name(dir);
                    }
                }
                if(_jump_menu.is_empty()) {
                    ImGui::Selectable("no subdirectories", false, ImGuiSelectableFlags_Disabled);
                }
                ImGui::EndPopup();
            }
        }

        ImGui::PopStyleVar();
        ImGui::PopStyleColor();
    }


    {
        ImGui::SameLine();

        const usize search_bar_size = 200;
        const usize margin = 24;

        const bool has_seach_bar = (ImGui::GetContentRegionAvail().x > margin + search_bar_size) && dynamic_cast<const SearchableFileSystemModel*>(filesystem());
        if(has_seach_bar) {
            ImGui::SameLine(ImGui::GetContentRegionMax().x - (search_bar_size + margin));
            ImGui::SetNextItemWidth(search_bar_size);
            if(imgui::text_input(ICON_FA_SEARCH, _search_pattern)) {
                update_search();
            }
        } else {
            ImGui::NewLine();
        }


        if(!has_seach_bar || _search_pattern.is_empty()) {
            _search_results = nullptr;
        }
    }


    ImGui::PopID();
}

void ResourceBrowser::draw_search_results() {
    _preview_id = AssetId::invalid_id();

    const ImGuiTableFlags table_flags = ImGuiTableFlags_RowBg;
    if(ImGui::BeginTable("##searchresults", 1, table_flags)) {
        for(const Entry& entry : *_search_results) {
            imgui::table_begin_next_row();
            if(ImGui::Selectable(fmt_c_str("{} {}", entry.icon.icon, entry.name))) {
                if(const AssetId id = asset_id(entry.name); id != AssetId::invalid_id()) {
                    asset_selected(id);
                }
                _set_path_deferred = entry.name;
            }

            if(ImGui::IsItemHovered()) {
                if(const AssetId id = asset_id(entry.name); id != AssetId::invalid_id()) {
                    _preview_id = id;
                }
            }
        }

        if(_search_results->is_empty()) {
            imgui::table_begin_next_row();
            ImGui::Selectable("No results", false, ImGuiSelectableFlags_Disabled);
        }

        ImGui::EndTable();
    }
}



void ResourceBrowser::on_gui() {
    y_profile();

    draw_top_bar();

    if(is_searching()) {
        draw_search_results();
    } else {
        FileSystemView::on_gui();
    }

    if(_set_path_deferred != path()) {
        set_path(_set_path_deferred);
        _search_results = nullptr;
    }
}






void ResourceBrowser::path_changed() {
    _set_path_deferred = path();
    _path_pieces.make_empty();

    core::String full_path = path();
    while(!full_path.is_empty()) {
        if(const auto parent = filesystem()->parent_path(full_path)) {
            const usize parent_size = parent.unwrap().size();
            if(parent_size < full_path.size()) {
                const bool has_separator = full_path[parent_size] == '/';
                _path_pieces << full_path.sub_str(parent_size + has_separator);
                full_path = parent.unwrap();
                continue;
            }
        }
        break;
    }

    _path_pieces << ""; // Root
}



void ResourceBrowser::update() {
    if(is_searching()) {
        update_search();
    }
    FileSystemView::update();
}

void ResourceBrowser::update_search() {
    _search_results = nullptr;

    if(const auto* searchable = dynamic_cast<const SearchableFileSystemModel*>(filesystem())) {
        std::string_view pattern = std::string_view(_search_pattern.data());
        const core::String full_pattern = fmt("{}%{}%", path(), pattern);

        if(auto result = searchable->search(full_pattern)) {
            _search_results = std::make_unique<core::Vector<Entry>>();

            for(const core::String& full_name : result.unwrap()) {
                const core::String name = searchable->filename(full_name);
                const bool is_dir = filesystem()->is_directory(full_name).unwrap_or(false);
                const EntryType type = is_dir ? EntryType::Directory : EntryType::File;
                if(auto icon = entry_icon(full_name, type)) {
                    _search_results->emplace_back(Entry {
                        full_name,
                        type,
                        std::move(icon.unwrap()),
                        usize(0)
                    });
                }
            }
        } else {
            log_msg("Search failed", Log::Warning);
        }
    }
}

core::Result<UiIcon> ResourceBrowser::entry_icon(const core::String& full_name, EntryType type) const {
    if(type == EntryType::Directory) {
        return FileSystemView::entry_icon(full_name, type);
    }
    if(const AssetId id = asset_id(full_name); id != AssetId::invalid_id()) {
        const AssetType asset = read_file_type(id);
        return core::Ok(UiIcon{asset_type_icon(asset), 0xFFFFFFFF});
    }
    return core::Err();
}

void ResourceBrowser::entry_hoverred(const Entry* entry) {
    _preview_id = AssetId::invalid_id();
    if(entry) {
        if(const AssetId id = asset_id(entry_full_name(*entry)); id != AssetId::invalid_id()) {
            _preview_id = id;
        }
    }
}

void ResourceBrowser::entry_clicked(const Entry& entry) {
    const core::String full_name = entry_full_name(entry);
    if(const AssetId id = asset_id(full_name); id != AssetId::invalid_id()) {
        asset_selected(id);
    }
    FileSystemView::entry_clicked(entry);
}

}

