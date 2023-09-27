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

#include "AssetStringifier.h"
#include "FileBrowser.h"

#include <yave/assets/AssetStore.h>
#include <yave/utils/FileSystemModel.h>
#include <yave/meshes/MeshData.h>

#include <y/utils/log.h>
#include <y/utils/format.h>
#include <y/serde3/archives.h>
#include <y/io2/File.h>

#include <editor/utils/ui.h>

namespace yave {

// implemented in DeviceResourcesData.cpp
MeshData cube_mesh_data();
MeshData sphere_mesh_data();
MeshData simple_sphere_mesh_data();
MeshData cone_mesh_data();

}

namespace editor {

static core::Result<void> export_to_obj(const MeshData& mesh, io2::Writer& writer) {
    auto write = [&](std::string_view l) {
        return writer.write(l.data(), l.size());
    };

    y_try_discard(write("# Yave OBJ export\n"));
    y_try_discard(write("o Mesh\n"));

    const usize vertex_count = mesh.vertex_streams().vertex_count();
    for(usize i = 0; i != vertex_count; ++i) {
        const FullVertex v = unpack_vertex(mesh.vertex_streams()[i]);
        y_try_discard(write(fmt("v {} {} {}\nvn {} {} {}\nvt {} {}\n",
            v.position.x(),
            v.position.y(),
            v.position.z(),
            v.normal.x(),
            v.normal.y(),
            v.normal.z(),
            v.uv.x(),
            v.uv.y()
        )));
    }

    y_try_discard(write("s 0\n"));

    for(const IndexedTriangle& triangle : mesh.triangles()) {
        const IndexedTriangle tri = {triangle[0] + 1, triangle[1] + 1, triangle[2] + 1};
        y_try_discard(write(fmt("f {}/{}/{} {}/{}/{} {}/{}/{}\n",
            tri[0], tri[0], tri[0],
            tri[1], tri[1], tri[1],
            tri[2], tri[2], tri[2]
        )));
    }
    return core::Ok();
}



AssetStringifier::AssetStringifier() :
        Widget("Asset stringifier"),
        _selector(AssetType::Mesh) {

    _selector.set_visible(false);
    _selector.set_selected_callback([this](AssetId id) { stringify(id); return true; });
}

AssetStringifier::~AssetStringifier() {
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
        imgui::text_read_only("##name", name);
    }

    _selector.draw_gui_inside();

    if(!_selector.is_visible()) {
        if(ImGui::Button("Cube")) {
            stringify(cube_mesh_data());
        }

        ImGui::SameLine();

        if(ImGui::Button("Sphere")) {
            stringify(sphere_mesh_data());
        }

        ImGui::SameLine();

        if(ImGui::Button("Simple sphere")) {
            stringify(simple_sphere_mesh_data());
        }

        ImGui::SameLine();

        if(ImGui::Button("Cone")) {
            stringify(cone_mesh_data());
        }
    }

    ImGui::Separator();

    {
        ImGui::Text("Vertex data: %u bytes", u32(_vertices.size()));
        ImGui::Spacing();
        if(ImGui::Button("Copy to clipboard##vertices")) {
            ImGui::SetClipboardText(_vertices.data());
        }
    }

    ImGui::Separator();

    {
        ImGui::Text("Triangle data: %u bytes", u32(_triangles.size()));
        ImGui::Spacing();
        if(ImGui::Button("Copy to clipboard##triangles")) {
            ImGui::SetClipboardText(_triangles.data());
        }
    }

    if(_mesh) {
        ImGui::Separator();
        if(ImGui::Button("Export to OBJ")) {
            FileBrowser* browser = add_detached_widget<FileBrowser>();
            browser->set_selection_filter("*.obj", FileBrowser::FilterFlags::AllowNewFiles);
            browser->set_selected_callback([mesh = _mesh](const auto& filename) {
                if(auto res = io2::File::create(filename)) {
                    if(export_to_obj(*mesh, res.unwrap()).is_ok()) {
                        log_msg(fmt("Exported as {}", filename));
                    } else {
                        log_msg("Export failed", Log::Error);
                    }
                } else {
                    log_msg(fmt("Unable to create file {}", filename), Log::Error);
                }
                return true;
            });
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
    stringify(std::move(mesh));
}


void AssetStringifier::stringify(MeshData mesh) {
    _vertices.make_empty();
    _triangles.make_empty();

    const int prec = 3;

    const usize vertex_count = mesh.vertex_streams().vertex_count();
    const auto positions = mesh.vertex_streams().stream<VertexStreamType::Position>();
    const auto normal_tangent = mesh.vertex_streams().stream<VertexStreamType::NormalTangent>();
    const auto uvs = mesh.vertex_streams().stream<VertexStreamType::Uv>();

    auto vertices = core::Vector<core::String>::with_capacity(vertex_count);
    for(usize i = 0; i != vertex_count; ++i) {
        std::array<char, 1024> buffer;
        std::snprintf(buffer.data(), buffer.size(), "{{%.*ff, %.*ff, %.*ff}, 0x%x, 0x%x, {%.*ff, %.*ff}}",
                      prec, positions[i].x(), prec, positions[i].y(), prec, positions[i].z(),
                      normal_tangent[i].x(),
                      normal_tangent[i].y(),
                      prec, uvs[i].x(), prec, uvs[i].y());
        vertices.emplace_back(buffer.data());
    };

    fmt_into(_vertices, "{}", vertices);
    fmt_into(_triangles, "{}", mesh.triangles());

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

    _mesh = std::make_shared<MeshData>(std::move(mesh));
}


void AssetStringifier::clear() {
    _selected = AssetId();
    _vertices.clear();
    _triangles.clear();
}

}

