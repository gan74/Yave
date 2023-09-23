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
#include "Entity.h"

namespace yave {

EntityId EntityPool::create_entity() {
    const EntityId id = create_id();

    _entities.set_min_size(id.index() + 1);
    Entity& e = _entities[id.index()];
    e.id = id;

    return id;
}

void EntityPool::remove_entity(EntityId id) {
    y_debug_assert(exists(id));

    Entity& e = _entities[id.index()];
    e = {};
    _free << id;
}

bool EntityPool::exists(EntityId id) const {
    return id.index() < _entities.size() && _entities[id.index()].id == id;
}

usize EntityPool::entity_count() const {
    y_debug_assert(_entities.size() >= _free.size());
    return _entities.size() - _free.size();
}

EntityId EntityPool::create_id() {
    EntityId id;
    if(!_free.is_empty()) {
        id = _free.pop();
    } else {
        id._index = u32(_entities.size());
    }

    ++id._generation;
    return id;
}


}



