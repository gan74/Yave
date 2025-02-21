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

#include "UndoStack.h"

#include "EditorWorld.h"
#include "Widget.h"

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <editor/utils/ui.h>

namespace editor {


class UndoStackDebug : public Widget {

    editor_widget_open(UndoStackDebug)

    public:
        UndoStackDebug() : Widget("Undo stack") {
        }

        void on_gui() override {
            const auto items = undo_stack().items();
            const usize top = undo_stack().stack_top();

            ImGui::Text("%u items in stack", u32(items.size()));

            if(ImGui::BeginTable("##undostack", 2, ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersInnerV)) {
                ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthStretch);
                ImGui::TableSetupColumn("Entity ID", ImGuiTableColumnFlags_WidthFixed);
                ImGui::TableHeadersRow();
                for(usize i = 0; i != items.size(); ++i) {
                    const usize index = items.size() - i - 1;
                    const auto& item = items[index];
                    const bool is_current =  index + 1 == top;

                    imgui::table_begin_next_row();
                    ImGui::Selectable(item.name.data(), is_current, ImGuiSelectableFlags_SpanAllColumns);

                    ImGui::TableNextColumn();
                    ImGui::Selectable(fmt_c_str("{:#08x}", item.id.index()));
                }

                ImGui::EndTable();
            }
        }
};


UndoStack::UndoStack() {
}


void UndoStack::undo() {
    if(!_top) {
        log_msg("Nothing to undo", Log::Warning);
        return;
    }

    --_top;
    _items[_top].undo(current_world(), _items[_top].id);

}

void UndoStack::redo() {
    if(_top == _items.size()) {
        log_msg("Nothing to redo", Log::Warning);
        return;
    }

    _items[_top].redo(current_world(), _items[_top].id);
    ++_top;
}

void UndoStack::push(Item item) {
    while(_items.size() != _top) {
        _items.pop();
    }
    _items.emplace_back(std::move(item));
    ++_top;
}

core::Span<UndoStack::Item> UndoStack::items() const {
    return _items;
}

usize UndoStack::stack_top() const {
    return _top;
}

}

