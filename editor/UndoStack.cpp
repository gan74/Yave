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

#include "UndoStack.h"

#include "EditorWorld.h"
#include "Widget.h"

#include <y/utils/log.h>

#include <editor/utils/ui.h>

namespace editor {

// editor_action_shortcut(ICON_FA_UNDO " Undo", Key::Ctrl + Key::Z, [] { undo_stack().undo(); }, "Edit")
// editor_action_shortcut(ICON_FA_REDO " Redo", Key::Ctrl + Key::Y, [] { undo_stack().redo(); }, "Edit")


/*void print_pos(const ecs::EntityPrefab& prefab) {
    for(const auto& comp : prefab.components()) {
        using CC = ecs::ComponentBox<TransformableComponent>;
        if(const auto* tr = dynamic_cast<const CC*>(comp.get())) {
            log_msg(fmt("{}", tr->component().position()));
        }
    }
}*/

class UndoStackWidget : public Widget {

    editor_widget(UndoStackWidget)

    public:
        UndoStackWidget() : Widget("Undo stack", ImGuiWindowFlags_AlwaysAutoResize) {
        }

        void on_gui() override {
#if 0
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


           ImGui::SameLine();

           if(ImGui::Button(ICON_FA_TRASH " Clear")) {
               undo_stack().clear();
           }

            if(ImGui::BeginListBox("##undostack")) {
                //const auto& stack = undo_stack()._stack;
                //const usize cursor = stack.size() - undo_stack()._cursor - 1;
                //for(usize i = 0; i != stack.size(); ++i) {
                //    ImGui::Selectable(fmt_c_str("Entity #{}", stack[i].id.index()), i == cursor);
                //}
                ImGui::EndListBox();
            }
#endif
        }
};


UndoStack::UndoStack() {
}

void UndoStack::clear() {
}

void UndoStack::set_editing_entity(ecs::EntityId) {
    y_profile();

}

void UndoStack::done_editing() {
}

bool UndoStack::is_entity_dirty() const {
    return false;
}

void UndoStack::push_before_dirty(ecs::EntityId) {
}

void UndoStack::make_dirty() {
}


bool UndoStack::can_undo() const {
    return false;
}

void UndoStack::undo() {
}

bool UndoStack::can_redo() const {
    return false;
}

void UndoStack::redo() {
}

void UndoStack::restore_entity() {
}

}

