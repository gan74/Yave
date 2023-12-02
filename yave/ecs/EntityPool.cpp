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
    y_debug_assert(!first_child.is_valid());
    y_debug_assert(!left_sibling.is_valid());
    y_debug_assert(!right_sibling.is_valid());
    id.invalidate();
}

void EntityPool::Entity::invalidate_hierarchy() {
    left_sibling = right_sibling = parent = {};
}

void EntityPool::Entity::make_valid(u32 index) {
    y_debug_assert(!is_valid());
    id.make_valid(index);
    left_sibling = right_sibling = parent = {};
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

void EntityPool::reroot_all_children(EntityId id, EntityId new_parent) {
    Y_TODO(Fix alloc, maybe use a recursive function)
    core::SmallVector<EntityId, 64> children_cached;
    for(const EntityId child : children(id)) {
        children_cached << child;
    }

    for(const EntityId child : children_cached) {
        _entities[child.index()].invalidate_hierarchy();
    }

    _entities[id.index()].first_child = {};

    if(new_parent.is_valid()) {
        for(const EntityId child : children_cached) {
            set_parent(child, new_parent);
        }
    }
}

void EntityPool::remove(EntityId id) {
    y_debug_assert(exists(id));

    auto& entity = _entities[id.index()];
    reroot_all_children(id, entity.parent);

    set_parent(id, {});

    entity.invalidate();
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

    y_always_assert(!is_parent(parent_id, id), "Entity hierarchy can not have cycles");

    y_debug_assert(child.parent.is_valid() == child.right_sibling.is_valid());
    y_debug_assert(child.left_sibling.is_valid() == child.right_sibling.is_valid());


    if(child.parent.is_valid()) {
        y_debug_assert(child.id.is_valid());

        const bool alone_child = child.right_sibling == child.id;

        Entity& parent = _entities[child.parent.index()];
        if(parent.first_child == child.id) {
            parent.first_child = alone_child ? EntityId() : child.right_sibling;
        }

        if(!alone_child) {
            Entity& left = _entities[child.left_sibling.index()];
            Entity& right = _entities[child.right_sibling.index()];

            left.right_sibling = right.id;
            right.left_sibling = left.id;
        }

        child.invalidate_hierarchy();
    }


    if(parent_id.is_valid()) {
        Entity& parent = _entities[parent_id.index()];
        if(parent.first_child.is_valid()) {
            Entity& other = _entities[parent.first_child.index()];
            y_debug_assert(other.parent == parent_id);

            y_debug_assert(child.id.is_valid() && other.id.is_valid());
            y_debug_assert(other.parent.is_valid());
            y_debug_assert(!child.left_sibling.is_valid());
            y_debug_assert(!child.right_sibling.is_valid());

            Entity& end = _entities[other.left_sibling.index()];
            y_debug_assert(end.right_sibling == other.id);

            end.right_sibling = child.id;
            child.left_sibling = end.id;

            other.left_sibling = child.id;
            child.right_sibling = other.id;

        } else {
            parent.first_child = child.id;
            child.left_sibling = child.right_sibling = child.id;
        }
        child.parent = parent_id;
    }

    audit();
}

bool EntityPool::is_parent(EntityId id, EntityId parent) const {
    if(!parent.is_valid() || !id.is_valid()) {
        return false;
    }
    for(const EntityId p : parents(id)) {
        if(p == parent) {
            return true;
        }
    }
    return false;
}

void EntityPool::audit() {
#ifdef Y_DEBUG
    y_profile();

    // Detect cycles
    for(const auto& en : _entities) {
        y_debug_assert(!is_parent(en.id, en.id));
    }

    // Parenting state
    for(const auto& en : _entities) {
        y_debug_assert(en.parent.is_valid() == en.right_sibling.is_valid());
        y_debug_assert(en.left_sibling.is_valid() == en.right_sibling.is_valid());
    }

#if 0
    // Children cycles & parent
    for(const auto& en : _entities) {
        if(en.is_valid()) {
            for(const EntityId c : children(en.id)) {
                y_debug_assert(_entities[c.index()].is_valid());
                y_debug_assert(_entities[c.index()].parent == en.id);
            }
        }
    }

    // Check childrens
    for(const auto& en : _entities) {
        if(en.is_valid() && en.parent.is_valid()) {
            bool found = false;
            for(const EntityId c : children(en.parent)) {
                found |= c == en.id;
            }
            y_debug_assert(found);
        }
    }
#endif
#endif
}

}
}

