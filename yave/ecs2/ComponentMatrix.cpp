/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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

#include "ComponentMatrix.h"
#include "EntityGroup.h"

namespace yave {
namespace ecs2 {

ComponentMatrix::ComponentMatrix(usize type_count) : _type_count(std::max(1u, u32(type_count))), _groups(_type_count) {
}

void ComponentMatrix::register_group(EntityGroupBase* group) {
    y_profile();

    for(const ComponentTypeIndex type : group->types()) {
        _groups[usize(type)] << group;
    }

    for(const EntityId id : _ids) {
        if(id.is_valid() && in_group(id, group)) {
            group->add_entity(id);
        }
    }
}

bool ComponentMatrix::in_group(EntityId id, const EntityGroupBase* group) const {
    for(const ComponentTypeIndex type : group->types()) {
        if(!has_component(id, type)) {
            return false;
        }
    }
    return true;
}

void ComponentMatrix::add_entity(EntityId id) {
    const u32 total = (id.index() + 1) * _type_count;
    _bits.set_min_size(total / 64 + 1);
    _ids.set_min_size(usize(id.index() + 1));
    y_debug_assert(!contains(id));
    _ids[id.index()] = id;
}

void ComponentMatrix::remove_entity(EntityId id) {
    y_debug_assert(contains(id));
    _ids[id.index()] = {};
}

void ComponentMatrix::add_component(EntityId id, ComponentTypeIndex type) {
    y_debug_assert(!has_component(id, type));

    const ComponentIndex index = component_index(id, type);
    _bits[index.index] |= index.mask;

    {
        y_profile_zone("updating groups");
        for(EntityGroupBase* group : _groups[usize(type)]) {
            if(in_group(id, group)) {
                group->add_entity(id);
            }
        }
    }
}

void ComponentMatrix::remove_component(EntityId id, ComponentTypeIndex type) {
    y_debug_assert(contains(id));
    y_debug_assert(has_component(id, type));

    const ComponentIndex index = component_index(id, type);
    _bits[index.index] &= ~index.mask;

    {
        y_profile_zone("updating groups");
        for(EntityGroupBase* group : _groups[usize(type)]) {
            y_debug_assert(!in_group(id, group));
            group->remove_entity(id);
        }
    }
}


bool ComponentMatrix::contains(EntityId id) const {
    return _ids.size() > id.index() && _ids[id.index()] == id;
}

bool ComponentMatrix::has_component(EntityId id, ComponentTypeIndex type) const {
    const ComponentIndex index = component_index(id, type);
    return index.index < _bits.size() && (_bits[index.index] & index.mask) != 0;
}

ComponentMatrix::ComponentIndex ComponentMatrix::component_index(EntityId id, ComponentTypeIndex type) const {
    const u32 index = id.index() * _type_count + u32(type);

    const u32 pack_index = index / 64;
    const u32 bit_index = index % 64;
    const u64 mask = u64(1) << bit_index;
    return {mask, pack_index};
}

}
}

