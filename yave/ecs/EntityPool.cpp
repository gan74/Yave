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

#include "EntityPool.h"

namespace yave {
namespace ecs {

bool EntityPool::Entity::is_valid() const {
    return id.is_valid();
}

void EntityPool::Entity::invalidate() {
    y_debug_assert(is_valid());
    y_debug_assert(!parent.is_valid());
    y_debug_assert(!left_sibling.is_valid());
    y_debug_assert(!right_sibling.is_valid());
    id.invalidate();
}

void EntityPool::Entity::make_valid(u32 index) {
    y_debug_assert(!is_valid());
    id.make_valid(index);
    left_sibling = right_sibling = id;
}


usize EntityPool::size() const {
    return _entities.size() - _free.size();
}

bool EntityPool::exists(EntityId id) const {
    return id.is_valid() &&
           id.index() < _entities.size() &&
           _entities[id.index()].id == id;
}

EntityId EntityPool::id_from_index(u32 index) const {
    if(index >= _entities.size() || !_entities[index].is_valid()) {
        return EntityId();
    }
    return _entities[index].id;
}

EntityId EntityPool::create() {
    u32 index = 0;
    if(_free.is_empty()) {
        index = u32(_entities.size());
        _entities.emplace_back();
    } else {
        index = _free.pop();
    }

    _entities[index].make_valid(index);
    return _entities[index].id;
}


void EntityPool::recycle(EntityId id) {
    y_debug_assert(exists(id));
    set_parent(id, {});
    _entities[id.index()].invalidate();
    _free << id.index();
}

EntityId EntityPool::first_child(EntityId id) const {
    y_debug_assert(exists(id));
    return _entities[id.index()].first_child;
}

EntityId EntityPool::next_child(EntityId id) const {
    y_debug_assert(exists(id));
    return _entities[id.index()].right_sibling;
}

EntityId EntityPool::parent(EntityId id) const {
    if(id.index() < _entities.size()) {
        return _entities[id.index()].parent;
    }
    return {};
}

void EntityPool::set_parent(EntityId id, EntityId parent_id) {
    y_debug_assert(exists(id));

    Entity& child = _entities[id.index()];
    if(child.parent == parent_id) {
        return;
    }

    // Unparent
    if(child.parent.is_valid()) {
        Entity& left = _entities[child.left_sibling.index()];
        Entity& right = _entities[child.right_sibling.index()];
        left.right_sibling = child.right_sibling;
        right.left_sibling = child.left_sibling;

        Entity& parent = _entities[child.parent.index()];
        if(parent.first_child == id) {
            parent.first_child = (child.right_sibling == id) ? EntityId() : child.right_sibling;
        }
    }

    // Set parent
    child.parent = parent_id;
    if(parent_id.is_valid()) {
        Entity& parent = _entities[parent_id.index()];
        const EntityId first_id = parent.first_child;
        parent.first_child = id;

        if(first_id.is_valid()) {
            Entity& first = _entities[first_id.index()];
            child.right_sibling = first_id;
            if(first.right_sibling == first_id) {
                first.right_sibling = id;
            }

            child.left_sibling = first.left_sibling;
            first.left_sibling = id;

        }
    }
}

void EntityPool::audit() {
#ifdef Y_DEBUG
    // Detect cycles
    for(const auto& en : _entities) {
        EntityId p = en.parent;
        while(p.is_valid()) {
            y_debug_assert(p != en.id);
            p = parent(p);
        }
    }

    // Children cycles & parent
    for(const auto& en : _entities) {
        EntityId c = en.first_child;
        if(c.is_valid()) {
            do {
                const Entity& e = _entities[c.index()];
                y_debug_assert(e.parent == en.id);
                c = e.right_sibling;
            } while(c != en.first_child);
        }
    }
#endif
}

}
}

