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

UndoStack::UndoStack() {
}

void UndoStack::clear() {
    _items.clear();
    _top = 0;
    _last_id = {};
}

void UndoStack::undo(bool merged) {
    if(!_top) {
        log_msg("Nothing to undo", Log::Warning);
        return;
    }

    y_profile();

    while(_top) {
        --_top;
        _items[_top].undo(current_world());
        if(!merged || !_items[_top].merge_with_prev) {
            break;
        }
    }

}

void UndoStack::redo(bool merged) {
    if(_top == _items.size()) {
        log_msg("Nothing to redo", Log::Warning);
        return;
    }

    y_profile();

    while(_top != _items.size()) {
        _items[_top].redo(current_world());
        ++_top;

        if(!merged || _top == _items.size() || !_items[_top].merge_with_prev) {
            break;
        }
    }
}

void UndoStack::push(core::String name, UndoFunc undo, UndoFunc redo, UndoId id) {
    y_profile();

    while(_items.size() != _top) {
        _items.pop();
    }

    y_debug_assert(id.id);

    const bool merge = id.id == _last_id.id && _since_last.elapsed() < merge_time;

    _last_id = id;
    _since_last.reset();

    _items.emplace_back(Item {
        std::move(name),
        std::move(undo),
        std::move(redo),
        merge
    });
    ++_top;
}

core::Span<UndoStack::Item> UndoStack::items() const {
    return _items;
}

usize UndoStack::stack_top() const {
    return _top;
}

UndoStack::UndoId UndoStack::generate_static_id() {
    static u32 next_id = 0;
    return { ++next_id };
}

}

