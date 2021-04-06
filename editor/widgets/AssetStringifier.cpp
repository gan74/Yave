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

#include "AssetStringifier.h"

#include <yave/assets/AssetStore.h>
#include <yave/utils/FileSystemModel.h>
#include <yave/meshes/MeshData.h>

#include <y/utils/log.h>
#include <y/utils/format.h>
#include <y/serde3/archives.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

AssetStringifier::AssetStringifier() :
        Widget("Asset stringifier"),
        _selector(AssetType::Mesh) {

    _selector.set_visible(false);
    _selector.set_selected_callback([this](AssetId id) { stringify(id); return true; });
}

void AssetStringifier::on_gui() {
    {
        if(ImGui::Button(ICON_FA_FOLDER_OPEN)) {
            _selector.set_visible(true);
        }

        ImGui::SameLine();

        const auto clean_name = [=](auto&& n) { return asset_store().filesystem()->filename(n); };
        core::String name = asset_store().name(_selected).map(clean_name).unwrap_or(core::String("No mesh"));

        ImGui::SetNextItemWidth(-1);
        ImGui::InputText("", name.data(), name.size(), ImGuiInputTextFlags_ReadOnly);
    }

    _selector.draw_gui_inside();

    if(_selected != AssetId::invalid_id()) {
        ImGui::Text("Vertex data:");
        ImGui::SameLine();
        ImGui::Spacing();
        if(ImGui::Button("Copy to clipboard##vertices")) {
            ImGui::SetClipboardText(_vertices.data());
        }

        ImGui::Text("Triangle data:");
        ImGui::SameLine();
        ImGui::Spacing();
        if(ImGui::Button("Copy to clipboard##triangles")) {
            ImGui::SetClipboardText(_triangles.data());
        }
    }
}

void AssetStringifier::stringify(AssetId id) {
    const auto data = asset_store().data(id);
    if(!data) {
        log_msg("Unable to find asset.", Log::Error);
        clear();
        return;
    }

    const core::DebugTimer timer("Stringify mesh");

    MeshData mesh;
    serde3::ReadableArchive arc(*data.unwrap());
    if(!arc.deserialize(mesh)) {
        log_msg("Unable to load asset.", Log::Error);
        clear();
        return;
    }

    _selected = id;
    _vertices.make_empty();
    _triangles.make_empty();

    const int prec = 3;

    auto vertices = core::vector_with_capacity<core::String>(mesh.vertices().size());
    for(const Vertex& v : mesh.vertices()) {
        std::array<char, 1024> buffer;
        std::snprintf(buffer.data(), buffer.size(), "{{%.*ff, %.*ff, %.*ff}, {%.*ff, %.*ff, %.*ff}, {%.*ff, %.*ff, %.*ff}, {%.*ff, %.*ff}}",
                      prec, v.position.x(), prec, v.position.y(), prec, v.position.z(),
                      prec, v.normal.x(), prec, v.normal.y(), prec, v.normal.z(),
                      prec, v.tangent.x(), prec, v.tangent.y(), prec, v.tangent.z(),
                      prec, v.uv.x(), prec, v.uv.y());
        vertices.emplace_back(buffer.data());
    };

    fmt_into(_vertices, "%", vertices);
    fmt_into(_triangles, "%", mesh.triangles());

    auto fix_brackets = [](char& c) {
            if(c == '[') {
                c = '{';
            }
            if(c == ']') {
                c = '}';
            }
        };

    for(char& c : _vertices) {
        fix_brackets(c);
    }
    for(char& c : _triangles) {
        fix_brackets(c);
    }
}


void AssetStringifier::clear() {
    _selected = AssetId();
    _vertices.clear();
    _triangles.clear();
}

}

