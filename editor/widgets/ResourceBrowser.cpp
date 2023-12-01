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

    ImGui::SameLine();
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

    core::String current_path = path();
    if(imgui::text_input("##path", current_path, ImGuiInputTextFlags_AutoSelectAll | ImGuiInputTextFlags_EnterReturnsTrue, "root")) {
        _set_path_deferred = current_path;
    }


    ImGui::PopID();
}

void ResourceBrowser::on_gui() {
    y_profile();

    draw_top_bar();
    FileSystemView::on_gui();

    if(_set_path_deferred != path()) {
        set_path(_set_path_deferred);
    }
}

void ResourceBrowser::path_changed() {
    _set_path_deferred = path();
}

void ResourceBrowser::update() {
    FileSystemView::update();
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


void ResourceBrowser::entry_clicked(const Entry& entry) {
    const core::String full_name = entry_full_name(entry);
    if(const AssetId id = asset_id(full_name); id != AssetId::invalid_id()) {
        asset_selected(id);
    }
    FileSystemView::entry_clicked(entry);
}

}

