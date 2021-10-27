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

#include "UndoStack.h"

#include "EditorWorld.h"

#include <y/utils/log.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

editor_action(ICON_FA_UNDO " Undo", [] { undo_stack().undo(); }, "Edit")
editor_action(ICON_FA_REDO " Redo", [] { undo_stack().redo(); }, "Edit")

UndoStack::UndoStack() {
}

void UndoStack::set_editing_entity(ecs::EntityId id) {
    y_profile();

    done_editing();

    if(id.is_valid() && id != _id) {
        _prefab = current_world().create_prefab(id);
    }
    _id = id;
}

void UndoStack::done_editing() {
    if(_pushed && _id.is_valid()) {
        _prefab = current_world().create_prefab(_id);
        _pushed = false;
    }
}

void UndoStack::make_dirty() {
    y_profile();

    if(!_pushed && _id.is_valid()) {
        while(_stack.size() > _cursor) {
            _stack.pop();
        }

        _stack.emplace_back(StackItem{_id, std::move(_prefab)});

        ++_cursor;
        _pushed = true;
    }
}

void UndoStack::undo() {
    y_profile();

    if(!_cursor) {
        log_msg("Undo stack is empty");
        return;
    }

    --_cursor;
    restore_entity();
}


void UndoStack::redo() {
    y_profile();

    if(_cursor == _stack.size()) {
        log_msg("Nothing to redo");
        return;
    }

    ++_cursor;
    restore_entity();
}


void UndoStack::restore_entity() {
    y_profile();

    const StackItem& item = _stack[_cursor];
    ecs::EntityWorld& world = current_world();
    for(const auto& comp : item.prefab.components()) {
        if(comp) {
            comp->add_to(world, item.id);
        }
    }
}

}

