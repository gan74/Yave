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

#include "entities.h"

#include <editor/components/EditorComponent.h>
#include <yave/components/TransformableComponent.h>

#include <editor/EditorWorld.h>
#include <editor/UndoStack.h>

namespace editor {

void undo_enabled_rename(ecs::EntityId target_id, const core::String& new_name) {
    y_profile();

    const EditorComponent* comp = current_world().component<EditorComponent>(target_id);
    if(!comp) {
        return;
    }

    static const auto undo_id = UndoStack::generate_static_id();
    undo_stack().push(
        "Entity renamed",
        [target_id, old_name = comp->name()](EditorWorld& w) {
            if(EditorComponent* comp = w.component_mut<EditorComponent>(target_id)) {
                comp->set_name(old_name);
            }
        },
        [target_id, new_name](EditorWorld& w) {
            if(EditorComponent* comp = w.component_mut<EditorComponent>(target_id)) {
                comp->set_name(new_name);
            }
        },
        undo_id,
        true
    );
}



static void move_recursive(EditorWorld& world, ecs::EntityId id, math::Transform<> old_parent_transform, math::Transform<> new_parent_transform) {
    if(TransformableComponent* component = world.component_mut<TransformableComponent>(id)) {
        const math::Transform<> tr = component->transform();
        component->set_transform(new_parent_transform * old_parent_transform.inverse() * tr);
        old_parent_transform = tr;
        new_parent_transform = component->transform();
    }

    for(const ecs::EntityId child : world.children(id)) {
        move_recursive(world, child, old_parent_transform, new_parent_transform);
    }
}

void undo_enabled_move_recursive(ecs::EntityId target_id, const math::Transform<> &new_transform) {
    y_profile();

    const TransformableComponent* comp = current_world().component<TransformableComponent>(target_id);
    if(!comp) {
        return;
    }

    const math::Transform<> old_transform = comp->transform();

    static const auto undo_id = UndoStack::generate_static_id();
    undo_stack().push(
        "Entity moved",
        [=](EditorWorld& w) { move_recursive(w, target_id, new_transform, old_transform); },
        [=](EditorWorld& w) { move_recursive(w, target_id, old_transform, new_transform); },
        undo_id,
        true
    );
}



void undo_enabled_add_component(ecs::EntityId target_id, ecs::ComponentRuntimeInfo info) {
    static const auto undo_id = UndoStack::generate_static_id();
    undo_stack().push(
        "Added component",
        [info, target_id](EditorWorld& w) {
            w.remove_component(target_id, info.type_id);
        },
        [info, target_id](EditorWorld& w) {
            info.add_or_replace_component(w, target_id);
        },
        undo_id,
        true
    );
}


void undo_enabled_remove_component(ecs::EntityId target_id, ecs::ComponentRuntimeInfo info) {
    static const auto undo_id = UndoStack::generate_static_id();
    undo_stack().push(
        "Remove component",
        [target_id, box = std::shared_ptr(current_world().create_box_from_component(target_id, info.type_id))](EditorWorld& w) {
            box->add_or_replace(w, target_id);
        },
        [target_id, info](EditorWorld& w) {
            w.remove_component(target_id, info.type_id);
        },
        undo_id,
        true
    );
}

}

