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

#include "SceneImporter.h"

#include <editor/components/EditorComponent.h>
#include <editor/import/transforms.h>
#include <editor/utils/assets.h>
#include <editor/utils/ui.h>

#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/meshes/MeshData.h>
#include <yave/animations/Animation.h>
#include <yave/material/Material.h>
#include <yave/graphics/images/ImageData.h>
#include <yave/assets/AssetLoader.h>
#include <yave/utils/FileSystemModel.h>
#include <yave/ecs/EntityWorld.h>

#include <y/io2/Buffer.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

SceneImporter::SceneImporter() : SceneImporter(FileSystemModel::local_filesystem()->current_path().unwrap_or("")) {
}

SceneImporter::SceneImporter(const core::String& import_path) :
        Widget("Mesh importer"),
        _import_path(import_path) {

    _browser.set_selection_filter(false, import::supported_scene_extensions());
    _browser.set_canceled_callback([this] { close(); return true; });
    _browser.set_selected_callback([this](const auto& filename) {
            _filename = filename;
            _state = State::Settings;
            return true;
        });
}

void SceneImporter::on_gui() {
    if(_state == State::Browsing) {
        _browser.draw_gui_inside();
    } else if(_state == State::Settings) {
        paint_import_settings();
    } else {
        if(done_loading()) {
            _state = State::Done;
            try {
                _import_future.get();
            } catch(std::exception& e) {
                log_msg(fmt("Unable to import scene: %" , e.what()), Log::Error);
            }
            close();
        } else {
            ImGui::Text("Loading...");
        }
    }
}

import::SceneImportFlags SceneImporter::scene_import_flags() const {
    using import::SceneImportFlags;

    SceneImportFlags flags = SceneImportFlags::ImportDiffuseAsSRGB;
    if(_import_meshes) {
        flags = flags | SceneImportFlags::ImportMeshes;
    }
    if(_import_anims) {
        flags = flags | SceneImportFlags::ImportAnims;
    }
    if(_import_images) {
        flags = flags | SceneImportFlags::ImportImages;
    }
    if(_import_materials) {
        flags = flags | SceneImportFlags::ImportMaterials;
    }
    if(_flip_uvs) {
        flags = flags | SceneImportFlags::FlipUVs;
    }
    if(_import_materials && _import_meshes) {
        flags = flags | SceneImportFlags::ImportPrefabs;
    }
    return flags;
}

void SceneImporter::paint_import_settings() {
    {
        if(imgui::path_selector("Import path:", _import_path)) {
            FileBrowser* browser = add_child_widget<FileBrowser>(asset_store().filesystem());
            browser->set_selection_filter(true);
            browser->set_selected_callback([this](const auto& filename) {
                    if(asset_store().filesystem()->is_directory(filename).unwrap_or(false)) {
                        _import_path = filename;
                        return true;
                    }
                    return false;
                });
        }
        ImGui::Separator();
    }

    {

        ImGui::Checkbox("Import meshes", &_import_meshes);
        ImGui::Checkbox("Import animations", &_import_anims);
        ImGui::Checkbox("Import images", &_import_images);
        ImGui::Checkbox("Import materials", &_import_materials);
        ImGui::Separator();

        const char* axes[] = {"+X", "-X", "+Y", "-Y", "+Z", "-Z"};
        if(ImGui::BeginCombo("Forward", axes[_forward_axis])) {
            for(usize i = 0; i != 6; ++i) {
                if(ImGui::Selectable(axes[i])) {
                    _forward_axis = i;
                    if(_forward_axis / 2 == _up_axis / 2) {
                        _up_axis = (_up_axis + 4) % 6;
                    }
                }
            }
            ImGui::EndCombo();
        }
        if(ImGui::BeginCombo("Up", axes[_up_axis])) {
            for(usize i = 0; i != 6; ++i) {
                if(ImGui::Selectable(axes[i])) {
                    _up_axis = i;
                    if(_forward_axis / 2 == _up_axis / 2) {
                        _forward_axis = (_forward_axis + 4) % 6;
                    }
                }
            }
            ImGui::EndCombo();
        }

        ImGui::Checkbox("Flip UVs", &_flip_uvs);
        ImGui::Separator();

        ImGui::DragFloat("Scale", &_scale);
        ImGui::Separator();

        // Update state
        {
            _import_materials &= _import_images;
        }
    }

    if(ImGui::Button("Ok")) {
        _state = State::Importing;
        _import_future = std::async(std::launch::async, [=] {
            import(import::import_scene(_filename, scene_import_flags()));
        });
    }
    ImGui::SameLine();
    if(ImGui::Button("Cancel")) {
        close();
    }
}

bool SceneImporter::done_loading() const {
    return _import_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready;
}

void SceneImporter::import(import::SceneData scene) {
    y_profile();

    auto make_full_name = [this](std::string_view import_path, std::string_view name) {
            core::String path = _import_path;
            if(!import_path.empty()) {
                path = asset_store().filesystem()->join(path, import_path);
            }
            return asset_store().filesystem()->join(path, name);
        };

    auto import_asset = [&](const auto& asset, std::string_view name, AssetType type) {
            log_msg(fmt("Saving asset as \"%\"", name));
            y_profile_zone("asset import");
            io2::Buffer buffer;
            serde3::WritableArchive arc(buffer);
            if(auto sr = arc.serialize(asset); sr.is_error()) {
                log_msg(fmt("Unable serialize \"%\": error %", name, sr.error().type), Log::Error);
            } else {
                buffer.reset();
                if(auto r = asset_store().import(buffer, name, type); r.is_error()) {
                    log_msg(fmt("Unable to import \"%\": error %", name, r.error()), Log::Error);
                }
            }
        };

    auto import_assets = [&](const auto& assets, std::string_view import_path, AssetType type) {
            for(const auto& a : assets) {
                const core::String name = make_full_name(import_path, a.name());
                import_asset(a.obj(), name, type);
            }
        };


    Y_TODO(try to auto detect handedness)
    if(_forward_axis != 0 || _up_axis != 4 || _scale != 1.0f) {
        const math::Vec3 axes[] = {
                {1.0f, 0.0f, 0.0f}, {-1.0f,  0.0f,  0.0f},
                {0.0f, 1.0f, 0.0f}, { 0.0f, -1.0f,  0.0f},
                {0.0f, 0.0f, 1.0f}, { 0.0f,  0.0f, -1.0f}
            };

        const math::Vec3 forward = axes[_forward_axis];
        const math::Vec3 up = axes[_up_axis];

        math::Transform<> transform;
        transform.set_basis(forward * _scale, forward.cross(up) * _scale, up * _scale);

        for(auto& mesh : scene.meshes) {
            mesh = import::transform(mesh.obj(), transform.transposed());
        }
    }

    // Apparently Assimp can't do this properly...
    for(auto& mesh : scene.meshes) {
        mesh = import::compute_tangents(mesh.obj());
    }


    {
        const bool separate_folders =
            (scene.meshes.is_empty() ? 0 : 1) +
            (scene.animations.is_empty() ? 0 : 1) +
            (scene.images.is_empty() ? 0 : 1) > 1;
        const core::String mesh_import_path = separate_folders ? "Meshes" : "";
        const core::String animations_import_path = separate_folders ? "Animations" : "";
        const core::String image_import_path = separate_folders ? "Textures" : "";
        const core::String material_import_path = separate_folders ? "Materials" : "";
        const core::String prefab_import_path = separate_folders ? "Prefabs" : "";;

        {
            import_assets(scene.meshes, mesh_import_path, AssetType::Mesh);
            import_assets(scene.animations, animations_import_path, AssetType::Animation);
            import_assets(scene.images, image_import_path, AssetType::Image);
        }


        {
            auto materials = core::vector_with_capacity<Named<SimpleMaterialData>>(scene.materials.size());
            for(const auto& mat : scene.materials) {
                const auto& data = mat.obj();
                SimpleMaterialData material;
                material.constants() = {data.emissive, data.roughness, data.metallic};
                for(usize i = 0; i != SimpleMaterialData::texture_count; ++i) {
                    if(!data.textures[i].is_empty()) {
                        const core::String tex_full_name = make_full_name(image_import_path, data.textures[i]);
                        if(const auto texture = asset_loader().load_res<Texture>(tex_full_name)) {
                            material.set_texture(SimpleMaterialData::Textures(i), std::move(texture.unwrap()));
                        } else {
                            log_msg(fmt("Unable to load texture \"%\"", tex_full_name), Log::Error);
                        }
                    }
                }
                materials.emplace_back(mat.name(), std::move(material));
            }

            import_assets(materials, material_import_path, AssetType::Material);
        }

        {
            auto prefabs = core::vector_with_capacity<Named<ecs::EntityPrefab>>(scene.prefabs.size());
            for(const auto& prefab : scene.prefabs) {
                bool ok = true;

                const auto& data = prefab.obj();
                const usize sub_mesh_count = data.sub_meshes.size();
                auto sub_meshes = core::vector_with_capacity<StaticMeshComponent::SubMesh>(sub_mesh_count);
                for(usize i = 0; i != data.sub_meshes.size(); ++i) {
                    const auto& obj = data.sub_meshes[i];

                    const core::String mesh_full_name = make_full_name(mesh_import_path, obj.mesh);
                    const core::String mat_full_name = make_full_name(material_import_path, obj.material);
                    auto mesh = asset_loader().load_res<StaticMesh>(mesh_full_name);
                    auto mat = asset_loader().load_res<Material>(mat_full_name);
                    if(mesh && mat) {
                        sub_meshes.emplace_back(StaticMeshComponent::SubMesh{mesh.unwrap(), mat.unwrap()});
                    } else {
                        ok = false;
                        break;
                    }
                }

                if(ok) {
                    ecs::EntityPrefab p;
                    log_msg(fmt("Adding % submeshes", sub_meshes.size()));
                    p.add(TransformableComponent());
                    p.add(StaticMeshComponent(std::move(sub_meshes)));

                    prefabs.emplace_back(prefab.name(), std::move(p));
                } else {
                    log_msg(fmt("Unable to create prefab for %", prefab.name()), Log::Error);
                }
            }

            import_assets(prefabs, prefab_import_path, AssetType::Prefab);
        }

    }

    refresh_all();
}

}

