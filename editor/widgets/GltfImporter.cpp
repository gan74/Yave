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

#include <yave/assets/AssetStore.h>
#include <yave/assets/AssetLoader.h>
#include <yave/ecs/EntityWorld.h>
#include <yave/components/TransformableComponent.h>

#include <editor/utils/ui.h>
#include <editor/components/EditorComponent.h>

#include <y/io2/Buffer.h>



namespace editor {

template<typename T>
static AssetId import_asset(AssetStore& store, const core::String& name, const T& asset) {
    io2::Buffer buffer;
    {
        y_profile_zone("serialize");
        serde3::WritableArchive arc(buffer);
        if(const auto res = arc.serialize(asset); res.is_error()) {
            return AssetId::invalid_id();
        }
        buffer.reset();
    }

    {
        y_profile_zone("import");
        if(const auto res = store.import(buffer, name, AssetTraits<T>::type); res.is_ok()) {
            return res.unwrap();
        }
    }

    return AssetId::invalid_id();
}

static AssetId import_node(AssetStore& store, import::ParsedScene& scene, int index) {
    if(index < 0 || scene.nodes[index].is_error) {
        return AssetId();
    }

    auto& node = scene.nodes[index];
    if(node.asset_id != AssetId::invalid_id()) {
        return node.asset_id;
    }

    ecs::EntityPrefab prefab(ecs::EntityId::dummy(u32(index)));
    prefab.add(EditorComponent(node.name));
    prefab.add(TransformableComponent(node.transform));

    for(const int child : node.children) {
        const AssetId id = import_node(store, scene, child);
        if(id != AssetId::invalid_id()) {
            prefab.add_child(make_asset_with_id<ecs::EntityPrefab>(id));
        }
    }

    node.asset_id = import_asset(store, node.name, prefab);
    node.is_error = node.asset_id == AssetId::invalid_id();

    return node.asset_id;
}



GltfImporter::GltfImporter() : GltfImporter(asset_store().filesystem()->current_path().unwrap_or(".")) {
}

GltfImporter::GltfImporter(std::string_view import_dst_path) :
        Widget("Scene importer"),
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
            core::DebugTimer _("Importing nodes");
            y_debug_assert(_scene.is_ok());
            for(const int node_index : _scene.unwrap().root_nodes) {
                import_node(asset_store(), _scene.unwrap(), node_index);
            }
            _state = State::Importing;
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

