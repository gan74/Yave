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

#include "EditorWorld.h"

#include <editor/components/EditorComponent.h>

#include <yave/ecs/EntityPrefab.h>
#include <yave/ecs/EntityPrefab.h>
#include <yave/components/DirectionalLightComponent.h>
#include <yave/components/SpotLightComponent.h>
#include <yave/components/PointLightComponent.h>
#include <yave/components/SkyLightComponent.h>
#include <yave/components/StaticMeshComponent.h>
#include <yave/components/TransformableComponent.h>

#include <yave/systems/AssetLoaderSystem.h>
#include <yave/systems/OctreeSystem.h>


#include <external/imgui/IconsFontAwesome5.h>

namespace editor {


std::string_view clean_component_name(std::string_view name) {
    usize start = 0;
    for(usize i = 0; i != name.size(); ++i) {
        switch(name[i]) {
            case ':':
                start = i + 1;
            break;

            default:
            break;
        }
    }

    return name.substr(start);
}


EditorWorld::EditorWorld(AssetLoader& loader) {
    add_required_component<EditorComponent>();
    add_system<AssetLoaderSystem>(loader);
    add_system<OctreeSystem>();
}

void EditorWorld::flush_reload() {
    AssetLoaderSystem* system = find_system<AssetLoaderSystem>();
    system->reset(*this);
}



bool EditorWorld::set_entity_name(ecs::EntityId id, std::string_view name) {
    if(EditorComponent* comp = component<EditorComponent>(id)) {
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

std::string_view EditorWorld::entity_icon(ecs::EntityId id) const {
    if(has<StaticMeshComponent>(id)) {
        return ICON_FA_CUBE;
    }

    if(has<PointLightComponent>(id)) {
        return ICON_FA_LIGHTBULB;
    }

    if(has<SpotLightComponent>(id)) {
        return ICON_FA_VIDEO;
    }

    if(has<DirectionalLightComponent>(id)) {
        return ICON_FA_SUN;
    }

    if(has<SkyLightComponent>(id)) {
        return ICON_FA_CLOUD;
    }

    return ICON_FA_DATABASE;
}

core::Span<std::pair<core::String, ecs::ComponentRuntimeInfo>> EditorWorld::component_types() {
    static core::Vector<std::pair<core::String, ecs::ComponentRuntimeInfo>> types;
    static bool init = false;

    if(!init) {
        for(const auto* poly_base = ecs::ComponentContainerBase::_y_serde3_poly_base.first; poly_base; poly_base = poly_base->next) {
            if(const auto container = poly_base->create()) {
                const ecs::ComponentRuntimeInfo info = container->runtime_info();
                types.emplace_back(clean_component_name(info.type_name), info);
            }
        }
        init = true;
    }

    return types;
}


}

