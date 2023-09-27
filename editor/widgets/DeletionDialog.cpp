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

#include "DeletionDialog.h"

#include <editor/EditorWorld.h>
#include <editor/components/EditorComponent.h>

#include <editor/utils/ui.h>

namespace editor {

DeletionDialog::DeletionDialog(ecs::EntityId id) :
        Widget("Confirm", ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoDocking),
        _id(id) {
   set_modal(true);
}

void DeletionDialog::on_gui() {
    EditorWorld& world = current_world();

    const EditorComponent* component = world.component<EditorComponent>(_id);
    y_debug_assert(component);

    ImGui::Text("Delete \"%s\"?", component->name().data());

    if(world.has_children(_id)) {
        const usize children_count = world.children(_id).count();
        ImGui::Checkbox(fmt_c_str("Delete {} children", children_count), &_delete_children);
    }

    if(ImGui::Button("Ok")) {
        y_profile_zone("deleting entities");

        const ecs::EntityId parent = world.parent(_id);
        const auto children = core::Vector<ecs::EntityId>::from_range(world.children(_id));
        for(ecs::EntityId id : children) {
            if(_delete_children) {
                world.remove_entity(id);
            } else {
                world.set_parent(id, parent);
            }
        }

        world.remove_entity(_id);
        close();
    }

    ImGui::SameLine();

    if(ImGui::Button("Cancel")) {
        close();
    }
}

}

