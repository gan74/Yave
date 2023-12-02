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

#include "EditorWorld.h"

#include <editor/components/EditorComponent.h>

#include <yave/assets/AssetLoader.h>
#include <yave/utils/FileSystemModel.h>

#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SkyLightComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>
#include <yave/components/AtmosphereComponent.h>

#include <yave/systems/AssetLoaderSystem.h>
#include <yave/systems/AABBUpdateSystem.h>
#include <yave/systems/OctreeSystem.h>
#include <yave/systems/ScriptSystem.h>
#include <yave/systems/RendererSystem.h>

#include <y/utils/format.h>

#include <external/imgui/IconsFontAwesome5.h>

#include <algorithm>

namespace editor {

editor_action("Remove all entities", [] { current_world().remove_all_entities(); })

EditorWorld::EditorWorld(AssetLoader& loader) {
    add_system<AssetLoaderSystem>(loader);
    add_system<AABBUpdateSystem>();
    add_system<OctreeSystem>();
    add_system<ScriptSystem>();
    add_system<RendererSystem>();
}

void EditorWorld::flush_reload() {
    AssetLoaderSystem* system = find_system<AssetLoaderSystem>();
    system->reset();
}

bool EditorWorld::set_entity_name(ecs::EntityId id, std::string_view name) {
    if(EditorComponent* comp = get_or_add_component<EditorComponent>(id)) {
        comp->set_name(name);
        return true;
    }
    return false;
}


std::string_view EditorWorld::entity_name(ecs::EntityId id) const {
    if(const EditorComponent* comp = component<EditorComponent>(id)) {
        return comp->name();
    }

    return "";
}

UiIcon EditorWorld::entity_icon(ecs::EntityId id) const {
    const u32 base_color = 0xFFBE9270;      // light blue
    const u32 mesh_color = 0xFF9C6CFF;      // Pink-ish
    const u32 folder_color = imgui::folder_icon_color;
    const u32 light_color = 0xFFFFFFFF;

    if(!exists(id)) {
        return { ICON_FA_PUZZLE_PIECE, base_color };
    }

    if(has<StaticMeshComponent>(id)) {
        return { ICON_FA_CUBE, mesh_color };
    }

    if(has<PointLightComponent>(id)) {
        return { ICON_FA_LIGHTBULB, light_color };
    }

    if(has<SpotLightComponent>(id)) {
        return { ICON_FA_VIDEO, light_color };
    }

    if(has<DirectionalLightComponent>(id)) {
        return { ICON_FA_SUN, light_color };
    }

    if(has<SkyLightComponent>(id)) {
        return { ICON_FA_CLOUD_SUN, base_color };
    }

    if(has<AtmosphereComponent>(id)) {
        return { ICON_FA_CLOUD, base_color };
    }

    if(has_children(id)) {
        return { ICON_FA_FOLDER_OPEN, folder_color };
    }

    return { ICON_FA_PUZZLE_PIECE, base_color };
}

ecs::EntityId EditorWorld::add_prefab(std::string_view name) {
    if(const auto id = asset_store().id(name)) {
        return add_prefab(id.unwrap());
    }
    return ecs::EntityId();
}

ecs::EntityId EditorWorld::add_prefab(AssetId asset) {
    y_profile();

    if(const auto prefab = asset_loader().load_res<ecs::EntityPrefab>(asset)) {
        const ecs::EntityId id = create_entity(*prefab.unwrap());

        if(EditorComponent* comp = get_or_add_component<EditorComponent>(id)) {
            comp->set_parent_prefab(asset);
        }
        if(const auto name = asset_store().name(asset)) {
            set_entity_name(id, asset_store().filesystem()->filename(name.unwrap()));
        }
        return id;
    }

    return ecs::EntityId();
}

bool EditorWorld::has_selected_entities() const {
    return !selected_entities().is_empty();
}

usize EditorWorld::selected_entity_count() const {
    return selected_entities().size();
}

core::Span<ecs::EntityId> EditorWorld::selected_entities() const {
    return with_tag(ecs::tags::selected);
}

bool EditorWorld::is_selected(ecs::EntityId id) const {
    return has_tag(id, ecs::tags::selected);
}

ecs::EntityId EditorWorld::selected_entity() const {
    const auto selected = selected_entities();
    return selected.size() == 1 ? selected[0] : ecs::EntityId();
}

void EditorWorld::set_selected(ecs::EntityId id) {
    clear_selection();
    if(id.is_valid()) {
        add_tag(id, ecs::tags::selected);
    }
}

void EditorWorld::toggle_selected(ecs::EntityId id, bool reset_selection) {
    if(!id.is_valid()) {
        return;
    }

    if(has_tag(id, ecs::tags::selected)) {
        if(!reset_selection) {
            remove_tag(id, ecs::tags::selected);
        }
    } else {
        if(reset_selection) {
            clear_selection();
        }
        add_tag(id, ecs::tags::selected);
    }
}

void EditorWorld::set_selection(core::Span<ecs::EntityId> selection) {
    clear_selection();
    for(const ecs::EntityId id : selection) {
        add_tag(id, ecs::tags::selected);
    }
}

void EditorWorld::clear_selection() {
    y_profile();
    clear_tag(ecs::tags::selected);
}

core::Span<std::pair<core::String, ecs::ComponentRuntimeInfo>> EditorWorld::component_types() {
    static core::Vector<std::pair<core::String, ecs::ComponentRuntimeInfo>> types;
    static bool init = false;

    if(!init) {
        for(const auto* poly_base = ecs::ComponentContainerBase::_y_serde3_poly_base.first; poly_base; poly_base = poly_base->next) {
            if(const auto container = poly_base->create()) {
                const ecs::ComponentRuntimeInfo info = container->runtime_info();
                types.emplace_back(info.clean_component_name(), info);
            }
        }
        init = true;
    }

    return types;
}



}

