/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#include <y/io2/Buffer.h>
#include <editor/utils/ui.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

template<typename T>
static usize count_errors(const core::Vector<T>& assets) {
    usize errors = 0;
    for(const T& asset : assets) {
        errors += asset.is_error;
    }
    return errors;
}

template<typename T>
static void set_global_import_flag(core::Vector<T>& assets) {
    ImGui::PushID(&assets);

    bool import = false;
    for(const T& asset : assets) {
        if(asset.import && !asset.is_error) {
            import = true;
            break;
        }
    }

    if(ImGui::Checkbox("##import", &import)) {
        for(T& asset : assets) {
            if(!asset.is_error) {
                asset.import = import;
            }
        }
    }

    ImGui::PopID();
}


static const char* make_display_name(const import::ParsedScene::Asset& asset) {
    return asset.is_error
        ? fmt_c_str(ICON_FA_EXCLAMATION_TRIANGLE " %", asset.name)
        : asset.name.data();
}

static bool patch_import_error(import::ParsedScene::Asset& asset) {
    asset.import &= !asset.is_error;
    if(asset.is_error) {
        ImGui::PushStyleColor(ImGuiCol_Text, imgui::error_text_color);
    }
    return asset.is_error;
}

template<typename T>
static void display_asset(T& asset) {
    ImGui::PushID(&asset);
    {
        const bool is_error = patch_import_error(asset);

        ImGui::Checkbox("##import", &asset.import);
        ImGui::SameLine();
        ImGui::Selectable(make_display_name(asset));

        if(!is_error) {
            if(ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
                add_child_widget<Renamer>(asset.name, [&](std::string_view new_name) { asset.name = new_name; return true; });
            }
        } else {
            ImGui::PopStyleColor();
            if(ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Asset \"%s\" could not be parsed and won't be imported", asset.name.data());
            }
        }
    }
    ImGui::PopID();
}

static void display_mesh_import_list(import::ParsedScene& scene) {
    set_global_import_flag(scene.meshes);
    ImGui::SameLine();

    if(ImGui::TreeNode(fmt_c_str("% Meshes", scene.meshes.size()))) {
        ImGui::Indent();

        for(auto& mesh : scene.meshes) {
            display_asset(mesh);
        }

        ImGui::Unindent();
        ImGui::TreePop();
    }
}

static void display_material_import_list(import::ParsedScene& scene) {
    set_global_import_flag(scene.materials);
    ImGui::SameLine();

    if(ImGui::TreeNode(fmt_c_str("% Materials", scene.materials.size()))) {
        ImGui::Indent();

        for(auto& material : scene.materials) {
            display_asset(material);
        }

        ImGui::Unindent();
        ImGui::TreePop();
    }
}

static void display_image_import_list(import::ParsedScene& scene) {
    set_global_import_flag(scene.images);
    ImGui::SameLine();

    if(ImGui::TreeNode(fmt_c_str("% Textures", scene.images.size()))) {
        ImGui::Indent();

        for(auto& image : scene.images) {
            display_asset(image);
        }

        ImGui::Unindent();
        ImGui::TreePop();
    }
}



SceneImporter2::SceneImporter2() : SceneImporter2(asset_store().filesystem()->current_path().unwrap_or(".")) {
}

SceneImporter2::SceneImporter2(std::string_view import_dst_path) :
        Widget("Scene importer"),
        _import_path(import_dst_path) {

    _browser.set_selection_filter(false, import::supported_scene_extensions());
    _browser.set_canceled_callback([this] { close(); return true; });
    _browser.set_selected_callback([this](const auto& filename) {
        _future = std::async(std::launch::async, [=] {
            y_profile_zone("parsing import");
            const auto kp = keep_alive();
            _scene = import::parse_scene(filename);
        });
        y_debug_assert(_state == State::Browsing);
        _state = State::Parsing;
        return true;
    });
}

template<typename T, typename L = decltype(log_msg)>
static AssetId import_single_asset(const T& asset, std::string_view name, AssetType type, L&& log_func = log_msg) {
    y_profile_zone("asset import");

    io2::Buffer buffer;
    serde3::WritableArchive arc(buffer);
    if(auto sr = arc.serialize(asset); sr.is_error()) {
        log_func(fmt("Unable serialize \"%\": error %", name, sr.error().type), Log::Error);
        return {};
    }

    buffer.reset();

    usize emergency_id = 1;
    core::String import_name = name;

    for(;;) {
        const auto result = asset_store().import(buffer, import_name, type);
        if(result.is_error()) {
            if(result.error() == AssetStore::ErrorType::NameAlreadyExists) {
                import_name = fmt("%_(%)", name, emergency_id++);
                continue;
            }
            log_func(fmt("Unable to import \"%\": error %", import_name, result.error()), Log::Error);
        } else {
            log_func(fmt("Saved asset as \"%\"", import_name), Log::Info);
            return result.unwrap();
        }
    }
}

void SceneImporter2::import_assets() {
    y_profile();

    auto log_func = [this](std::string_view msg, Log type) {
        log_msg(msg, type);
        const auto lock = y_profile_unique_lock(_lock);
        _logs.emplace_back(msg, type);
    };

    // --------------------------------- Images ---------------------------------
    {
        const core::String image_import_path = asset_store().filesystem()->join(_import_path, "Textures");
        for(usize i = 0; i != _scene.images.size(); ++i) {
            auto& image = _scene.images[i];
            if(image.import) {
                y_debug_assert(image.asset_id == AssetId::invalid_id());
                if(auto res = _scene.build_image_data(i)) {
                    image.asset_id = import_single_asset(res.unwrap(), asset_store().filesystem()->join(image_import_path, image.name), AssetType::Image, log_func);
                }
            }
        }
    }

    // --------------------------------- Meshes ---------------------------------
    {
        const core::String mesh_import_path = asset_store().filesystem()->join(_import_path, "Meshes");
        for(usize i = 0; i != _scene.meshes.size(); ++i) {
            auto& mesh = _scene.meshes[i];
            if(mesh.import) {
                y_debug_assert(mesh.asset_id == AssetId::invalid_id());
                core::Vector<core::Result<MeshData>> sub_meshes = _scene.build_mesh_data(i);
                for(usize j = 0; j != sub_meshes.size(); ++j) {
                    y_debug_assert(mesh.sub_meshes[j].asset_id == AssetId::invalid_id());
                    if(auto& res = sub_meshes[j]) {
                        mesh.sub_meshes[j].asset_id = import_single_asset(std::move(res.unwrap()), asset_store().filesystem()->join(mesh_import_path, mesh.name + "_" + mesh.sub_meshes[j].name), AssetType::Mesh, log_func);
                    }
                }
            }
        }
    }

    // --------------------------------- Materials ---------------------------------
    {
        const core::String material_import_path = asset_store().filesystem()->join(_import_path, "Materials");
        for(usize i = 0; i != _scene.materials.size(); ++i) {
            auto& material = _scene.materials[i];
            if(material.import) {
                y_debug_assert(material.asset_id == AssetId::invalid_id());
                if(auto res = _scene.build_material_data(i)) {
                    material.asset_id = import_single_asset(res.unwrap(), asset_store().filesystem()->join(material_import_path, material.name), AssetType::Material, log_func);
                }
            }
        }
    }

    // --------------------------------- Prefabs ---------------------------------
    auto build_prefab = [&](const import::ParsedScene::Mesh& mesh) {
        auto sub_meshes = core::vector_with_capacity<StaticMeshComponent::SubMesh>(mesh.sub_meshes.size());
        for(const auto& sub_mesh : mesh.sub_meshes) {
            if(sub_mesh.gltf_material_index < 0 || sub_mesh.asset_id == AssetId::invalid_id()) {
                continue;
            }

            AssetId material_id;
            for(const auto& mat : _scene.materials) {
                if(mat.gltf_index == sub_mesh.gltf_material_index) {
                    material_id = mat.asset_id;
                    break;
                }
            }

            if(material_id == AssetId::invalid_id()) {
                continue;
            }

            sub_meshes.emplace_back(make_asset_with_id<StaticMesh>(sub_mesh.asset_id), make_asset_with_id<Material>(material_id));
        }

        ecs::EntityPrefab prefab;
        prefab.add(TransformableComponent());
        prefab.add(StaticMeshComponent(std::move(sub_meshes)));
        return prefab;
    };

    {
        const core::String prefab_import_path = asset_store().filesystem()->join(_import_path, "Prefabs");
        for(auto& mesh : _scene.meshes) {
            if(mesh.import) {
                y_debug_assert(mesh.asset_id == AssetId::invalid_id());
                mesh.asset_id = import_single_asset(build_prefab(mesh), asset_store().filesystem()->join(prefab_import_path, mesh.name), AssetType::Prefab, log_func);
            }
        }
    }


    // --------------------------------- Scene ---------------------------------
    if(_scene.import_scene) {
        const core::String scene_import_path = asset_store().filesystem()->join(_import_path, "Scenes");
        auto prefabs = core::vector_with_capacity<ecs::EntityPrefab>(_scene.meshes.size());
        for(const auto& mesh : _scene.meshes) {
            if(mesh.asset_id != AssetId::invalid_id()) {
                prefabs << build_prefab(mesh);
            }
        }
        import_single_asset(ecs::EntityScene(std::move(prefabs)), asset_store().filesystem()->join(scene_import_path, _scene.name), AssetType::Scene, log_func);
    }
}

void SceneImporter2::on_gui() {
    const float button_height = ImGui::GetFont()->FontSize + ImGui::GetStyle().FramePadding.y * 2.0f + 4.0f;

    switch(_state) {
        case State::Browsing:
            _browser.draw_gui_inside();
        break;

        case State::Parsing:
            ImGui::Text("Parsing scene%s", imgui::ellipsis());
            ImGui::Checkbox("Import using default settings", &_import_with_default_settings);
            if(_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                _future.get();
                if(_scene.is_error) {
                    _state = State::Browsing;
                } else {
                    _state = State::Settings;
                }
            }
        break;

        case State::Settings: {
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
                const usize total_errors =
                    count_errors(_scene.meshes) +
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

            ImGui::Checkbox("Import as scene", &_scene.import_scene);

            ImGui::Separator();

            if(ImGui::BeginChild("##assets", ImVec2(0, ImGui::GetContentRegionAvail().y - button_height), true)) {
                display_mesh_import_list(_scene);
                display_material_import_list(_scene);
                display_image_import_list(_scene);
            }
            ImGui::EndChild();

            if(ImGui::Button(ICON_FA_CHECK " Import") || _import_with_default_settings) {
                _future = std::async(std::launch::async, [this] {
                    core::DebugTimer timer("Importing");
                    const auto kp = keep_alive();
                    import_assets();
                });
                _state = State::Importing;
            }
        } break;

        case State::Importing:
            ImGui::Text("Importing assets%s", imgui::ellipsis());
            if(_future.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                _future.get();
                refresh_all();
                _state = State::Done;
            }
            [[fallthrough]];

        case State::Done: {
            if(ImGui::BeginChild("##logs", ImVec2(0, ImGui::GetContentRegionAvail().y - button_height), true)) {
                imgui::alternating_rows_background();

                const auto lock = y_profile_unique_lock(_lock);
                for(const auto& log : _logs) {
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
            ImGui::EndChild();

            if(ImGui::Button("Ok")) {
                close();
            }
        } break;
    }
}

}

