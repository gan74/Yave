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


void Entity::init(EntityId id) {
    y_debug_assert(!is_valid());
    y_debug_assert(!_id.is_valid());
    y_debug_assert(_components.is_empty());
    _id = id;
}

void Entity::invalidate() {
    *this = {};
}

bool Entity::is_valid() const {
    return _id.is_valid();
}

EntityId Entity::id() const {
    return _id;
}

core::Span<Entity::Component> Entity::components() const {
    y_debug_assert(is_valid());
    return _components;
}

UntypedComponentRef Entity::get(ComponentType type) const {
    y_debug_assert(is_valid());
    const ComponentTypeIndex index = type.index();
    const auto it = find_type_it(index);
    if(it == _components.end() || it->type_index != index) {
        return {};
    }

    return it->component;
}

void Entity::register_component(UntypedComponentRef ref) {
    y_debug_assert(is_valid());
    const ComponentTypeIndex index = ref.type().index();
    const auto it = find_type_it(index);
    y_always_assert(it == _components.end() || it->type_index != index, "Component already exist");

    _components.insert(it, Component(index, ref));
}

UntypedComponentRef Entity::unregister_component(ComponentType type) {
    y_debug_assert(is_valid());
    const ComponentTypeIndex index = type.index();
    const auto it = find_type_it(index);
    y_always_assert(it != _components.end() && it->type_index == index, "Component doesn't exist");

    UntypedComponentRef ref = it->component;
    _components.erase(it);
    return ref;
}



Entity& EntityContainer::create_entity() {
    const EntityId id = create_id();
    _entities.set_min_size(id._index + 1);

    Entity& e = _entities[id._index];
    y_debug_assert(!e.is_valid());
    e.init(id);
    return e;
}

void EntityContainer::remove_entity(EntityId id) {
    y_debug_assert(exists(id));
    _to_remove << id;
}

bool EntityContainer::exists(EntityId id) const {
    return id._index < _entities.size() && _entities[id._index]._id == id;
}

void EntityContainer::register_component(EntityId id, UntypedComponentRef ref) {
    y_debug_assert(exists(id));
    _entities[id._index].register_component(ref);
}

UntypedComponentRef EntityContainer::unregister_component(EntityId id, ComponentType type) {
    y_debug_assert(exists(id));
    return _entities[id._index].unregister_component(type);
}

core::Span<Entity> EntityContainer::entities() const {
    return _entities;
}

const Entity& EntityContainer::operator[](EntityId id) const {
    y_debug_assert(exists(id));
    return _entities[id._index];
}

usize EntityContainer::entity_count() const {
    y_debug_assert(_entities.size() >= _free.size());
    return _entities.size() - _free.size();
}

void EntityContainer::flush_removals() {
    for(const EntityId id : _to_remove) {
        _entities[id._index].invalidate();
        _free << id;
    }
    _to_remove.make_empty();
}

EntityId EntityContainer::create_id() {
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



