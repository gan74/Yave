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
    return !_ids.is_empty();
}

core::Span<ecs::EntityId> Selection::selected_entities() const {
    return _ids;
}

ecs::EntityId Selection::selected_entity() const {
    if(_ids.size() != 1) {
        return ecs::EntityId();
    }
    return _ids.first();
}

bool Selection::is_selected(ecs::EntityId id) const {
    return std::find(_ids.begin(), _ids.end(), id) != _ids.end();
}

void Selection::add_or_remove(ecs::EntityId id, bool set) {
    if(!id.is_valid() || selected_entity() == id) {
        return;
    }

    if(const auto it = std::find(_ids.begin(), _ids.end(), id); it != _ids.end()) {
        if(set) {
            set_selected(id);
        } else {
            _ids.erase(it);
        }
    } else {
        if(set) {
            _ids.clear();
        }
        _ids << id;
    }
}

void Selection::set_selected(ecs::EntityId id) {
    _ids.clear();
    if(id.is_valid()) {
        _ids << id;
    }
}

void Selection::set_selected(core::Span<ecs::EntityId> ids) {
    _ids.clear();
    for(const ecs::EntityId id : ids) {
        if(id.is_valid()) {
            _ids << id;
        }
    }
}

void Selection::clear_selection() {
    _ids.clear();
}

}

