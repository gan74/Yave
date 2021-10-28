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
#include "Widget.h"

#include <y/utils/log.h>

#include <external/imgui/yave_imgui.h>

namespace editor {

editor_action_shortcut(ICON_FA_UNDO " Undo", Key::Ctrl + Key::Z, [] { undo_stack().undo(); }, "Edit")
editor_action_shortcut(ICON_FA_REDO " Redo", Key::Ctrl + Key::Y, [] { undo_stack().redo(); }, "Edit")


/*void print_pos(const ecs::EntityPrefab& prefab) {
    for(const auto& comp : prefab.components()) {
        using CC = ecs::ComponentBox<TransformableComponent>;
        if(const auto* tr = dynamic_cast<const CC*>(comp.get())) {
            log_msg(fmt("%", tr->component().position()));
        }
    }
}*/

class UndoStackWidget : public Widget {

    editor_widget(UndoStackWidget)

    public:
        UndoStackWidget() : Widget("Undo stack", ImGuiWindowFlags_AlwaysAutoResize) {
        }

        void on_gui() override {
            {
                ImGui::BeginDisabled(!undo_stack().can_undo());
                if(ImGui::Button(ICON_FA_UNDO " Undo")) {
                    undo_stack().undo();
                }
                ImGui::EndDisabled();
            }

            ImGui::SameLine();

            {
                ImGui::BeginDisabled(!undo_stack().can_redo());
                if(ImGui::Button(ICON_FA_REDO " Redo")) {
                    undo_stack().redo();
                }
                ImGui::EndDisabled();
            }

            if(ImGui::BeginListBox("##undostack")) {
                const auto& stack = undo_stack()._stack;
                const usize cursor = stack.size() - undo_stack()._cursor - 1;
                for(usize i = 0; i != stack.size(); ++i) {
                    ImGui::Selectable(fmt_c_str("Entity #%", stack[i].id.index()), i == cursor);
                }
                ImGui::EndListBox();
            }
        }
};


UndoStack::UndoStack() {
}

void UndoStack::set_editing_entity(ecs::EntityId id) {
    y_profile();

    if(_id == id) {
        return;
    }

    _id = id;
    _dirty = false;
    if(_id.is_valid()) {
        _stack.emplace_back(StackItem{_id, current_world().create_prefab(_id)});
    }
}

void UndoStack::done_editing() {
    if(_dirty && _id.is_valid()) {
        _stack.emplace_back(StackItem{_id, current_world().create_prefab(_id)});
        _dirty = false;
    }
}

bool UndoStack::is_entity_dirty() const {
    return _dirty;
}

void UndoStack::push_before_dirty(ecs::EntityId id) {
    if(id == _id) {
        make_dirty();
    } else {
        while(_cursor) {
            _stack.pop();
            --_cursor;
        }
        _stack.emplace_back(StackItem{_id, current_world().create_prefab(_id)});
    }
}

void UndoStack::make_dirty() {
    y_profile();

    if(!_dirty && _id.is_valid()) {
        while(_cursor) {
            _stack.pop();
            --_cursor;
        }

        _dirty = true;
    }
}


bool UndoStack::can_undo() const {
    return !(_cursor + 1 >= _stack.size());
}

void UndoStack::undo() {
    y_profile();

    if(!can_undo()) {
        log_msg("Nothing to undo");
        return;
    }

    ++_cursor;
    restore_entity();
}

bool UndoStack::can_redo() const {
    return _cursor;
}

void UndoStack::redo() {
    y_profile();

    if(!can_redo()) {
        log_msg("Nothing to redo");
        return;
    }

    --_cursor;
    restore_entity();
}

void UndoStack::restore_entity() {
    y_profile();

    const StackItem& item = _stack[_stack.size() - (_cursor + 1)];
    ecs::EntityWorld& world = current_world();
    for(const auto& comp : item.prefab.components()) {
        if(comp) {
            comp->add_to(world, item.id);
        }
    }
}

}

