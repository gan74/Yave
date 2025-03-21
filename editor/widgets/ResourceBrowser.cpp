/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include <editor/ImGuiPlatform.h>
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

ResourceBrowser::ResourceBrowser(std::string_view title) : Widget(title), _fs_view(asset_store().filesystem()) {
    _fs_view.set_split_mode_enabled(true);

    _fs_view.set_clicked_delegate([this](const core::String& full_name, FileSystemModel::EntryType type) {
        if(type == FileSystemModel::EntryType::File) {
            if(const AssetId id = asset_id(full_name); id != AssetId::invalid_id()) {
                return _selected_delegate(id);
            }
        }
        return false;
    });

    _fs_view.set_preview_delegate([this](const core::String& full_name, FileSystemModel::EntryType type) -> UiTexture {
        if(type == FileSystemModel::EntryType::File) {
            if(const AssetId id = asset_id(full_name); id != AssetId::invalid_id()) {
                if(const TextureView* tex = thumbmail_renderer().thumbmail(id)) {
                    return imgui_platform()->to_ui(*tex);
                }
            }
        }
        return {};
    });

    _fs_view.set_tooltip_delegate([this](const core::String& full_name, FileSystemModel::EntryType type) {
        if(type == FileSystemModel::EntryType::File) {
            if(const AssetId id = asset_id(full_name); id != AssetId::invalid_id()) {
                const std::string_view type_name = asset_type_name(asset_type(id), false, false);
                ImGui::TextUnformatted(type_name.data(), type_name.data() + type_name.size());
                ImGui::Separator();
            }
        }

        ImGui::TextUnformatted(full_name.data(), full_name.data() + full_name.size());
    });

}

AssetId ResourceBrowser::asset_id(std::string_view name) const {
    return asset_store().id(name).unwrap_or(AssetId());
}

AssetType ResourceBrowser::asset_type(AssetId id) const {
    return asset_store().asset_type(id).unwrap_or(AssetType::Unknown);
}

void ResourceBrowser::draw_import_menu() {
    if(ImGui::Selectable("Import glTF")) {
        add_detached_widget<GltfImporter>(_fs_view.path());
    }
    if(ImGui::Selectable("Import image")) {
        add_detached_widget<ImageImporter>(_fs_view.path());
    }
}

void ResourceBrowser::on_gui() {
    {
        ImGui::PushID("##pathbar");

        if(ImGui::Button(ICON_FA_PLUS " Import")) {
            ImGui::OpenPopup("##importmenu");
        }

        if(ImGui::BeginPopup("##importmenu")) {
            draw_import_menu();
            ImGui::EndPopup();
        }

        ImGui::PopID();

        ImGui::SameLine();
        if(ImGui::Button(ICON_FA_ARROW_UP)) {
            const FileSystemModel* fs = _fs_view.filesystem();
            _fs_view.set_path(fs->parent_path(_fs_view.path()).unwrap_or(_fs_view.root_path()));
        }

        ImGui::SameLine();
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        draw_path_bar();
    }

    _fs_view.draw_gui_inside();
}


void ResourceBrowser::draw_path_bar() {
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4{});
    const auto res = draw_path_bar_element(_fs_view.path());
    ImGui::PopStyleColor();
    if(res) {
        _fs_view.set_path(res.unwrap());
    }
}

core::Result<core::String> ResourceBrowser::draw_path_bar_element(std::string_view path) {
    if(path.empty()) {
        if(ImGui::Button("root")) {
            return core::Ok(_fs_view.root_path());
        }
        ImGui::SameLine();
        ImGui::TextUnformatted(ICON_FA_CHEVRON_RIGHT);
        return core::Err();
    }

    const FileSystemModel* fs = _fs_view.filesystem();
    if(const auto parent = fs->parent_path(path)) {
        if(auto res = draw_path_bar_element(parent.unwrap())) {
            return res;
        }
        ImGui::SameLine();

        const usize parent_len = parent.unwrap().size();
        const bool has_seperator = path[parent_len] == '/';
        if(has_seperator) {
            ImGui::TextUnformatted(ICON_FA_CHEVRON_RIGHT);
            ImGui::SameLine();
        }

        const std::string_view filename = path.substr(has_seperator ? parent_len + 1 : parent_len);
        if(ImGui::Button(fmt_c_str("{}", filename))) {
            return core::Ok(core::String(path));
        }
    }
    return core::Err();
}

}

