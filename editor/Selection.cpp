/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#include "Selection.h"

#include "UndoStack.h"

namespace editor {

bool Selection::has_selected_entities() const {
    return !_unordered.is_empty();
}

usize Selection::selected_entity_count() const {
    return _unordered.size();
}

ecs::EntityId Selection::selected_entity() const {
    if(_unordered.size() != 1) {
        return ecs::EntityId();
    }
    return _unordered.first();
}

bool Selection::is_selected(ecs::EntityId id) const {
    const auto it = std::lower_bound(_ordered.begin(), _ordered.end(), id);
    return it != _ordered.end() && it->id == id;
}

void Selection::add_or_remove(ecs::EntityId id, bool set) {
    if(!id.is_valid() || selected_entity() == id) {
        return;
    }

    if(const auto it = std::lower_bound(_ordered.begin(), _ordered.end(), id); it != _ordered.end() && it->id == id) {
        if(set) {
            set_selected(id);
        } else {
            const u32 index = it->index;
            _unordered.erase(_unordered.begin() + index);
            _ordered.erase(it);
            for(auto& p : _ordered) {
                if(p.index > index) {
                    --p.index;
                }
            }
        }
    } else {
        if(set) {
            clear_selection();
        }
        add(id);
    }
}

void Selection::add(ecs::EntityId id) {
    const auto it = std::lower_bound(_ordered.begin(), _ordered.end(), id);
    _ordered.insert(it, id, u32(_unordered.size()));
    _unordered.push_back(id);

    y_debug_assert(std::is_sorted(_ordered.begin(), _ordered.end()));
    y_debug_assert(std::all_of(_ordered.begin(), _ordered.end(), [this](const auto& p) { return _unordered[p.index] == p.id; }));
    y_debug_assert(_ordered.size() == _unordered.size());
}

void Selection::set_selected(ecs::EntityId id) {
    clear_selection();
    if(id.is_valid()) {
        add(id);
    }
}

void Selection::set_selected(core::Span<ecs::EntityId> ids) {
    clear_selection();
    for(const ecs::EntityId id : ids) {
        if(id.is_valid()) {
            _ordered.emplace_back(id, u32(_unordered.size()));
            _unordered.push_back(id);
        }
    }
    std::sort(_ordered.begin(), _ordered.end());
}

void Selection::clear_selection() {
   _ordered.clear();
   _unordered.clear();
}

}

