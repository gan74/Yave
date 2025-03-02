/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include "DeletionDialog.h"

#include <editor/EditorWorld.h>
#include <editor/components/EditorComponent.h>

#include <editor/utils/ui.h>

namespace editor {

static void remove_children(EditorWorld& world, ecs::EntityId id) {
    core::Vector<ecs::EntityId> children;
    for(const ecs::EntityId child : world.entity_pool().children(id)) {
        children << child;
    }

    for(const ecs::EntityId child : children) {
        remove_children(world, child);
        world.remove_entity(child);
    }
}

DeletionDialog::DeletionDialog(core::Span<ecs::EntityId> ids) :
        Widget("Confirm", ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking),
        _ids(ids) {

   set_modal(true);
}

void DeletionDialog::on_gui() {
    EditorWorld& world = current_world();

    const bool exists = std::any_of(_ids.begin(), _ids.end(), [&](const ecs::EntityId id) { return world.exists(id); });
    if(!exists) {
        close();
    }

    if(_ids.size() == 1) {
        const EditorComponent* component = world.component<EditorComponent>(_ids[0]);
        y_debug_assert(component);
        ImGui::Text("Delete \"%s\"?", component->name().data());
    } else {
        ImGui::Text("Delete %u entities?", u32(_ids.size()));
    }

    const bool has_children = std::any_of(_ids.begin(), _ids.end(), [&](const ecs::EntityId id) { return world.has_children(id); });
    if(has_children) {
        ImGui::Checkbox("Delete children", &_delete_children);
    }

    if(ImGui::Button("Ok")) {
        y_profile_zone("deleting entities");

        core::FixedArray<ecs::EntityPrefab> prefabs(_ids.size());
        std::transform(_ids.begin(), _ids.end(), prefabs.begin(), [&](ecs::EntityId id) { return world.create_prefab_from_entity(id); });

        for(const ecs::EntityId id : _ids) {
            if(!world.exists(id)) {
                continue;
            }
            if(_delete_children) {
                remove_children(world, id);
            }
            world.remove_entity(id);
        }

        close();
    }

    ImGui::SameLine();

    if(ImGui::Button("Cancel")) {
        close();
    }
}

}

