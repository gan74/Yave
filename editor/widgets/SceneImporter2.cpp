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

#include "SceneImporter2.h"
#include "Renamer.h"

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
#include <yave/ecs/EntityScene.h>

#include <editor/utils/ui.h>
#include <editor/components/EditorComponent.h>

#include <y/concurrent/StaticThreadPool.h>
#include <y/io2/Buffer.h>



namespace editor {

template<typename T>
static usize count_errors(const core::Vector<T>& assets) {
    usize errors = 0;
    for(const T& asset : assets) {
        errors += asset.is_error;
    }
    return errors;
}


SceneImporter2::SceneImporter2() : SceneImporter2(asset_store().filesystem()->current_path().unwrap_or(".")) {
}

SceneImporter2::SceneImporter2(std::string_view import_dst_path) :
        Widget("Scene importer"),
        _import_path(import_dst_path),
        _thread_pool(4u) {

    _browser.set_selection_filter(import::supported_scene_extensions());
    _browser.set_canceled_callback([this] { close(); return true; });
    _browser.set_selected_callback([this](const auto& filename) {
        _thread_pool.schedule([=] {
            y_profile_zone("parsing import");
            _scene = import::parse_scene(filename);
        });
        y_debug_assert(_state == State::Browsing);
        _state = State::Parsing;
        return true;
    });
}

bool SceneImporter2::should_keep_alive() const {
     return !_thread_pool.is_empty();
}

template<typename T, typename L = decltype(log_msg)>
static AssetId import_single_asset(const T& asset, std::string_view name, AssetType type, std::atomic<u32>& emergency_uid, L&& log_func = log_msg) {
    y_profile();

    io2::Buffer buffer;
    {
        y_profile_zone("serialize");
        serde3::WritableArchive arc(buffer);
        if(auto sr = arc.serialize(asset); sr.is_error()) {
            log_func(fmt("Unable serialize \"%\": error %", name, sr.error().type), Log::Error);
            return {};
        }
    }

    buffer.reset();

    core::String import_name = name;

    for(;;) {
        const auto result = asset_store().import(buffer, import_name, type);
        if(result.is_error()) {
            if(result.error() == AssetStore::ErrorType::NameAlreadyExists) {
                import_name = fmt("%_%", name, ++emergency_uid);
                continue;
            }
            log_func(fmt("Unable to import \"%\": error %", import_name, result.error()), Log::Error);
        } else {
            log_func(fmt("Saved asset as \"%\"", import_name), Log::Info);
            return result.unwrap();
        }
    }
}

usize SceneImporter2::import_assets() {
    y_profile();

    auto log_func = [this](std::string_view msg, Log type) {
        log_msg(msg, type);
        const auto lock = y_profile_unique_lock(_lock);
        _logs.emplace_back(msg, type);
    };

    concurrent::DependencyGroup image_deps;
    concurrent::DependencyGroup mesh_material_deps;
    usize to_import = 0;

    // --------------------------------- Images ---------------------------------
    if(_import_options.import_textures) {
        const core::String image_import_path = asset_store().filesystem()->join(_import_path, "Textures");
        for(usize i = 0; i != _scene.images.size(); ++i) {
            ++to_import;
            _thread_pool.schedule([=] {
                auto& image = _scene.images[i];
                y_debug_assert(image.asset_id == AssetId::invalid_id());
                if(image.is_error) {
                    return;
                }
                if(auto res = _scene.build_image_data(i, _import_options.compress_textues)) {
                    image.asset_id = import_single_asset(res.unwrap(), asset_store().filesystem()->join(image_import_path, image.name), AssetType::Image, _emergency_uid, log_func);
                }
            }, &image_deps);
        }
    }

    // --------------------------------- Meshes ---------------------------------
    if(_import_options.import_meshes) {
        const core::String mesh_import_path = asset_store().filesystem()->join(_import_path, "Meshes");
        for(usize i = 0; i != _scene.mesh_prefabs.size(); ++i) {
            ++to_import;
            _thread_pool.schedule([=] {
                auto& mesh = _scene.mesh_prefabs[i];
                y_debug_assert(mesh.mesh_asset_id == AssetId::invalid_id());
                if(mesh.is_error) {
                    return;
                }
                if(auto res = _scene.build_mesh_data(i)) {
                    mesh.mesh_asset_id = import_single_asset(
                        std::move(res.unwrap()),
                        asset_store().filesystem()->join(mesh_import_path, mesh.name + "_" + mesh.name),
                        AssetType::Mesh, _emergency_uid, log_func
                    );
                }
            }, &mesh_material_deps);
        }
    }

    // --------------------------------- Materials ---------------------------------
    if(_import_options.import_materials) {
        const core::String material_import_path = asset_store().filesystem()->join(_import_path, "Materials");
        for(usize i = 0; i != _scene.materials.size(); ++i) {
            ++to_import;
            _thread_pool.schedule([=] {
                auto& material = _scene.materials[i];
                y_debug_assert(material.asset_id == AssetId::invalid_id());
                if(material.is_error) {
                    return;
                }
                if(auto res = _scene.build_material_data(i)) {
                    material.asset_id = import_single_asset(res.unwrap(), asset_store().filesystem()->join(material_import_path, material.name), AssetType::Material, _emergency_uid, log_func);
                }
             }, &mesh_material_deps);
        }
    }

    // --------------------------------- Prefabs ---------------------------------
    auto build_prefab = [&](const import::ParsedScene::MeshPrefab& mesh, const math::Transform<>& transform = {}) {
        core::Vector<AssetPtr<Material>> materials(mesh.materials.size(), AssetPtr<Material>());
        for(usize i = 0; i != materials.size(); ++i) {
            if(mesh.materials[i].gltf_material_index < 0) {
                continue;
            }

            AssetId material_id;
            for(const auto& mat : _scene.materials) {
                if(mat.gltf_index == mesh.materials[i].gltf_material_index) {
                    material_id = mat.asset_id;
                    break;
                }
            }

            if(material_id == AssetId::invalid_id()) {
                continue;
            }

            materials[i] = make_asset_with_id<Material>(material_id);
        }

        ecs::EntityPrefab prefab;
        y_debug_assert(transform[3][3] == 1.0f);
        prefab.add(TransformableComponent(transform));
        prefab.add(StaticMeshComponent(make_asset_with_id<StaticMesh>(mesh.mesh_asset_id), std::move(materials)));
        return prefab;
    };

    if(_import_options.import_prefabs) {
        const core::String prefab_import_path = asset_store().filesystem()->join(_import_path, "Prefabs");
        for(usize i = 0; i != _scene.mesh_prefabs.size(); ++i) {
            ++to_import;
            _thread_pool.schedule([=] {
                auto& mesh = _scene.mesh_prefabs[i];
                y_debug_assert(mesh.asset_id == AssetId::invalid_id());
                if(mesh.is_error) {
                    return;
                }
                mesh.asset_id = import_single_asset(build_prefab(mesh), asset_store().filesystem()->join(prefab_import_path, mesh.name), AssetType::Prefab, _emergency_uid, log_func);
            }, nullptr, mesh_material_deps);
        }
    }


    // --------------------------------- Scene ---------------------------------
    if(_import_options.import_scene) {
        ++to_import;
        _thread_pool.schedule([=] {
            const core::String scene_import_path = asset_store().filesystem()->join(_import_path, "Scenes");
            auto prefabs = core::vector_with_capacity<ecs::EntityPrefab>(_scene.nodes.size());
            for(const auto& node : _scene.nodes) {
                const auto it = std::find_if(_scene.mesh_prefabs.begin(), _scene.mesh_prefabs.end(), [=](const auto& mesh) { return mesh.gltf_index == node.mesh_gltf_index; });
                if(it != _scene.mesh_prefabs.end()) {
                    ecs::EntityPrefab& prefab = prefabs.emplace_back(build_prefab(*it, node.transform));
                    prefab.add(EditorComponent(node.name));
                }
            }
            import_single_asset(ecs::EntityScene(std::move(prefabs)), asset_store().filesystem()->join(scene_import_path, _scene.name), AssetType::Scene, _emergency_uid, log_func);
        }, nullptr, mesh_material_deps);
    }

    return to_import;
}

void SceneImporter2::on_gui() {
    const float button_height = ImGui::GetFrameHeightWithSpacing();

    switch(_state) {
        case State::Browsing:
            _browser.draw_gui_inside();
        break;

        case State::Parsing:
            ImGui::Text("Parsing scene%s", imgui::ellipsis());
            ImGui::Checkbox("Import using default settings", &_import_with_default_settings);
            if(_thread_pool.is_empty()) {
                if(_scene.is_error) {
                    _state = State::Browsing;
                } else {
                    _state = State::Settings;
                }
            }

            if(ImGui::Button("Cancel")) {
                _thread_pool.cancel_pending_tasks();
                close();
            }
        break;

        case State::Settings: {
            {
                if(imgui::path_selector("Import path:", _import_path)) {
                    FileBrowser* browser = add_child_widget<FileBrowser>(asset_store().filesystem());
                    browser->set_selection_filter("", FileBrowser::FilterFlags::IncludeDirs);
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
                const usize total_errors =
                    count_errors(_scene.mesh_prefabs) +
                    count_errors(_scene.materials) +
                    count_errors(_scene.images);

                if(_scene.is_error) {
                    ImGui::TextColored(imgui::error_text_color, ICON_FA_EXCLAMATION_TRIANGLE " Error while parsing scene, some elements might be missing or broken");
                    ImGui::Separator();
                } else if(total_errors) {
                    ImGui::TextColored(imgui::warning_text_color, ICON_FA_EXCLAMATION_TRIANGLE " %u assets could not be parsed and won't be imported", u32(total_errors));
                    ImGui::Separator();
                }
            }

            ImGui::Checkbox("Import as scene", &_import_options.import_scene);
            ImGui::Checkbox(fmt_c_str("Import % prefabs", _scene.mesh_prefabs.size()), &_import_options.import_prefabs);
            ImGui::Checkbox(fmt_c_str("Import % meshes", _scene.mesh_prefabs.size()), &_import_options.import_meshes);
            ImGui::Checkbox(fmt_c_str("Import % materials", _scene.materials.size()), &_import_options.import_materials);
            ImGui::Checkbox(fmt_c_str("Import % textures", _scene.images.size()), &_import_options.import_textures);

            ImGui::Separator();

            ImGui::Checkbox("Compress textures", &_import_options.compress_textues);

            _import_options.update_flags();

            if(ImGui::Button(ICON_FA_CHECK " Import") || _import_with_default_settings) {
                _to_import = import_assets();
                _state = State::Importing;
            }
        } break;

        case State::Importing: {
            ImGui::TextUnformatted("Importing assets:");
            ImGui::SameLine();
            const usize imported = _to_import - _thread_pool.pending_tasks();
            ImGui::ProgressBar(float(imported) / float(_to_import), ImVec2(-1.0f, 0.0f), fmt_c_str("% / % assets imported", imported, _to_import));
            if(_thread_pool.is_empty()) {
                refresh_all();
                _state = State::Done;
            }
        } [[fallthrough]];

        case State::Done: {
            if(ImGui::CollapsingHeader("Details")) {
                if(ImGui::BeginTable("##logs", 1, ImGuiTableFlags_RowBg, ImVec2(0, ImGui::GetContentRegionAvail().y - button_height))) {
                    const auto lock = y_profile_unique_lock(_lock);
                    for(const auto& log : _logs) {
                        imgui::table_begin_next_row();
                        switch(log.second) {
                            case Log::Error:
                                ImGui::PushStyleColor(ImGuiCol_Text, imgui::error_text_color);
                                ImGui::Selectable(fmt_c_str(ICON_FA_EXCLAMATION_CIRCLE " %", log.first.data()));
                                ImGui::PopStyleColor();
                            break;

                            case Log::Warning:
                                ImGui::PushStyleColor(ImGuiCol_Text, imgui::error_text_color);
                                ImGui::Selectable(fmt_c_str(ICON_FA_EXCLAMATION_TRIANGLE " %", log.first.data()));
                                ImGui::PopStyleColor();
                            break;

                            default:
                                ImGui::Selectable(fmt_c_str(ICON_FA_CHECK " %", log.first.data()));
                        }
                    }
                }
                ImGui::EndTable();
            }

            if(ImGui::Button("Ok")) {
                close();
            }
            if(!_thread_pool.is_empty()) {
                ImGui::SameLine();
                if(ImGui::Button("Cancel")) {
                    _thread_pool.cancel_pending_tasks();
                    close();
                }
            }
        } break;
    }
}

}

