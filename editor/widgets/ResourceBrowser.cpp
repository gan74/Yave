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

ResourceBrowser::ResourceBrowser(std::string_view title) : Widget(title), _filesystem_view(asset_store().filesystem()) {
    _filesystem_view.set_split_mode(true);

    _filesystem_view.set_clicked_delegate([this](const core::String& full_name, FileSystemModel::EntryType type) {
        if(type == FileSystemModel::EntryType::File) {
            if(const AssetId id = asset_id(full_name); id != AssetId::invalid_id()) {
                return _selected_delegate(id);
            }
        }
        return false;
    });

    _filesystem_view.set_preview_delegate([this](const core::String& full_name, FileSystemModel::EntryType type) -> UiTexture {
        if(type == FileSystemModel::EntryType::File) {
            if(const AssetId id = asset_id(full_name); id != AssetId::invalid_id()) {
                if(const TextureView* tex = thumbmail_renderer().thumbmail(id)) {
                    return *tex;
                }
            }
        }
        return {};
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
        add_detached_widget<GltfImporter>(_filesystem_view.path());
    }
    if(ImGui::Selectable("Import image")) {
        add_detached_widget<ImageImporter>(_filesystem_view.path());
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
        ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);

        imgui::text_read_only("##currentpath", fmt("/{}", _filesystem_view.path()));
    }

    _filesystem_view.draw_gui_inside();
}

}

