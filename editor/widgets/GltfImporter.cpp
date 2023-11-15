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

#include "GltfImporter.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/device/DeviceResources.h>
#include <yave/assets/AssetStore.h>
#include <yave/assets/AssetLoader.h>
#include <yave/ecs/EntityWorld.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/meshes/StaticMesh.h>
#include <yave/material/Material.h>

#include <editor/utils/ui.h>
#include <editor/components/EditorComponent.h>

#include <y/io2/Buffer.h>



namespace editor {

template<typename T>
static AssetId import_asset(const core::String& name, const T& asset, AssetType type, const core::String& import_path) {
    y_profile();
    y_profile_msg(fmt_c_str("Importing {} {}", asset_type_name(type), name));

    io2::Buffer buffer;
    {
        y_profile_zone("serialize");
        serde3::WritableArchive arc(buffer);
        if(const auto res = arc.serialize(asset); res.is_error()) {
            log_msg(fmt("Unable to serialize {}", name), Log::Error);
            return AssetId::invalid_id();
        }
        buffer.reset();
    }

    {
        y_profile_zone("import");

        core::String suffix;
        for(usize i = 0;; ++i) {
            if(const auto res = asset_store().import(buffer, asset_store().filesystem()->join(import_path, name + suffix), type); res.is_ok()) {
                log_msg(fmt("Imported {} \"{}\" as {}", asset_type_name(type), name, stringify_id(res.unwrap())));
                return res.unwrap();
            } else if(res.error() == AssetStore::ErrorType::NameAlreadyExists) {
                suffix = fmt_to_owned(" ({})", i);
            } else {
                log_msg(fmt("Unable to import {}, error: {}", name, res.error()), Log::Error);
                break;
            }
        }

    }

    return AssetId::invalid_id();
}

static AssetId import_node(import::ParsedScene& scene, int index, bool import_child_prefab_as_assets, const core::String& import_path);

static std::pair<std::unique_ptr<ecs::EntityPrefab>, core::String> create_prefab(import::ParsedScene& scene, int index, bool import_child_prefabs_as_assets, const core::String& import_path) {
    if(index < 0 || scene.nodes[index].is_error) {
        return {};
    }

    auto& node = scene.nodes[index];

    core::String name = node.name;
    auto prefab = std::make_unique<ecs::EntityPrefab>(ecs::EntityId::dummy(u32(index)));

    if(node.mesh_index >= 0) {
        if(const auto& mesh = scene.meshes[node.mesh_index]; mesh.asset_id != AssetId::invalid_id()) {
            core::Vector<AssetPtr<Material>> materials;
            std::transform(mesh.materials.begin(), mesh.materials.end(), std::back_inserter(materials), [&](int mat_index) {
                if(mat_index < 0) {
                    return AssetPtr<Material>();
                }
                return make_asset_with_id<Material>(scene.materials[mat_index].asset_id);
            });

            prefab->add(StaticMeshComponent(make_asset_with_id<StaticMesh>(mesh.asset_id), std::move(materials)));

            name = mesh.name;
        }
    }

    for(const int child_index : node.children) {
        if(import_child_prefabs_as_assets) {
            if(const AssetId id = import_node(scene, child_index, import_child_prefabs_as_assets, import_path); id != AssetId::invalid_id()) {
                prefab->add_child(make_asset_with_id<ecs::EntityPrefab>(id));
            }
        } else {
            if(auto child = create_prefab(scene, child_index, import_child_prefabs_as_assets, import_path); child.first) {
                prefab->add_child(std::move(child.first));
            }
        }
    }

    prefab->add(TransformableComponent(node.transform));
    prefab->add(EditorComponent(name));

    return {std::move(prefab), name};
}

static AssetId import_node(import::ParsedScene& scene, int index, bool import_child_prefabs_as_assets, const core::String& import_path) {
    if(index < 0 || scene.nodes[index].is_error) {
        return AssetId();
    }

    auto& node = scene.nodes[index];
    if(node.asset_id != AssetId::invalid_id()) {
        return node.asset_id;
    }

    const auto [prefab, name] = create_prefab(scene, index, import_child_prefabs_as_assets, import_path);
    node.set_id(import_asset(name, *prefab, AssetType::Prefab, import_path));

    return node.asset_id;
}

template<typename T>
static void import_all(concurrent::StaticThreadPool& thread_pool, import::ParsedScene& scene, T settings) {
    concurrent::DependencyGroup image_group;
    concurrent::DependencyGroup material_group;
    concurrent::DependencyGroup mesh_group;

    for(usize i = 0; i != scene.images.size(); ++i) {
        thread_pool.schedule([i, settings, &scene] {
            auto& image = scene.images[i];
            if(const auto image_data = scene.create_image(int(i), true)) {
                image.set_id(import_asset(image.name, image_data.unwrap(), AssetType::Image, settings.import_path));
            }
        }, &image_group);
    }

    for(usize i = 0; i != scene.materials.size(); ++i) {
        thread_pool.schedule([i, settings, &scene] {
            auto& material = scene.materials[i];
            if(const auto material_data = scene.create_material(int(i))) {
                material.set_id(import_asset(material.name, material_data.unwrap(), AssetType::Material, settings.import_path));
            }
        }, &material_group, image_group);
    }

    for(usize i = 0; i != scene.meshes.size(); ++i) {
        thread_pool.schedule([i, settings, &scene] {
            auto& mesh = scene.meshes[i];
            if(const auto mesh_data = scene.create_mesh(int(i))) {
                mesh.set_id(import_asset(mesh.name, mesh_data.unwrap(), AssetType::Mesh, settings.import_path));
            }
        }, &mesh_group, material_group);
    }

    Y_TODO(slow, be import_node is not thread safe)
    thread_pool.schedule([settings, &scene] {
        import_node(scene, scene.root_node, settings.import_child_prefabs_as_assets, settings.import_path);
    }, nullptr, mesh_group);
}



GltfImporter::GltfImporter() : GltfImporter(asset_store().filesystem()->current_path().unwrap_or(".")) {
}

GltfImporter::GltfImporter(std::string_view import_dst_path) :
        Widget("glTF importer"),
        _import_path(import_dst_path),
        _thread_pool(4_uu) {

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

bool GltfImporter::should_keep_alive() const {
     return !_thread_pool.is_empty();
}

void GltfImporter::on_gui() {
    switch(_state) {
        case State::Browsing:
            _browser.draw_gui_inside();
        break;

        case State::Parsing:
            ImGui::Text("Parsing scene%s", imgui::ellipsis());
            if(_thread_pool.is_empty()) {
                if(_scene.is_error()) {
                    _state = State::Failed;
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
            y_debug_assert(_scene.is_ok());

            if(imgui::path_selector("Import path:", _settings.import_path)) {
                FileBrowser* browser = add_child_widget<FileBrowser>(asset_store().filesystem());
                browser->set_selection_filter("", FileBrowser::FilterFlags::IncludeDirs);
                browser->set_selected_callback([this](const auto& filename) {
                    if(asset_store().filesystem()->is_directory(filename).unwrap_or(false)) {
                        _settings.import_path = filename;
                        return true;
                    }
                    return false;
                });
            }

            ImGui::Checkbox("Import children prefabs as assets", &_settings.import_child_prefabs_as_assets);

            if(ImGui::Button(ICON_FA_CHECK " Import")) {
                import_all(_thread_pool, _scene.unwrap(), _settings);
                _state = State::Importing;
            }
        } break;

        case State::Importing: {
            ImGui::TextUnformatted("Importing...");
            if(_thread_pool.is_empty()) {
                refresh_all();
                _state = State::Done;
            } else {
                if(ImGui::Button("Cancel")) {
                    _thread_pool.cancel_pending_tasks();
                    close();
                }
            }
        } break;

        case State::Done: {
            ImGui::TextUnformatted("Done!");

            if(ImGui::Button("Ok")) {
                close();
            }
        } break;


        case State::Failed: {
            ImGui::TextUnformatted("Failed!");

            if(ImGui::Button("Ok")) {
                close();
            }
        } break;
    }
}

}

