/*******************************
Copyright (c) 2016-2026 Grégoire Angerand

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

#include "AssetReferenceWidget.h"

#include <yave/assets/AssetStore.h>
#include <yave/utils/FileSystemModel.h>

#include <y/utils/format.h>

#include <editor/utils/ui.h>

namespace editor {

static core::String asset_display_name(AssetId id) {
    const auto clean_name = [](auto&& n) { return asset_store().filesystem()->filename(n); };
    return asset_store().name(id).map(clean_name).unwrap_or(stringify_id(id));
}

AssetReferenceWidget::AssetReferenceWidget(AssetId id) :
        Widget("Asset references"),
        _id(id) {
}

void AssetReferenceWidget::on_gui() {
    ImGui::TextUnformatted(asset_display_name(_id).data());
    ImGui::Separator();

    if(ImGui::CollapsingHeader("References", ImGuiTreeNodeFlags_DefaultOpen)) {
        if(const auto refs = asset_store().references(_id)) {
            if(refs.unwrap().is_empty()) {
                ImGui::TextDisabled("None");
            }
            for(const AssetId ref : refs.unwrap()) {
                ImGui::BulletText("%s", asset_display_name(ref).data());
            }
        } else {
            ImGui::TextDisabled("Unavailable");
        }
    }

    if(ImGui::CollapsingHeader("Referenced by", ImGuiTreeNodeFlags_DefaultOpen)) {
        core::Vector<AssetId> refs;
        const bool ok = asset_store().search_references(_id, [&](AssetId ref) { refs << ref; }).is_ok();
        if(ok) {
            if(refs.is_empty()) {
                ImGui::TextDisabled("None");
            } else {
                for(const AssetId ref : refs) {
                    ImGui::BulletText("%s", asset_display_name(ref).data());
                }
            }
        } else {
            ImGui::TextDisabled("Unavailable");
        }
    }
}

}
