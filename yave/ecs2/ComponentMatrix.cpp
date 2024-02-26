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

namespace yave {
namespace ecs2 {

ComponentMatrix::ComponentMatrix(usize type_count) : _type_count(type_count) {
}

void ComponentMatrix::add_component(EntityId id, ComponentTypeIndex type, u32 slot_index) {
    _slots.set_min_size(usize(id.index() + 1) * _type_count);

    y_debug_assert(!has_component(id, type));
    _slots[component_index(id, type)].index = slot_index;
}

void ComponentMatrix::remove_component(EntityId id, ComponentTypeIndex type) {
    _slots.set_min_size(usize(id.index() + 1) * _type_count);

    y_debug_assert(has_component(id, type));
    _slots[component_index(id, type)].index = u32(-1);
}

bool ComponentMatrix::has_component(EntityId id, ComponentTypeIndex type) const {
    const usize index = component_index(id, type);
    return index < _slots.size() && _slots[index].index != u32(-1);
}

u32 ComponentMatrix::component_slot_index(EntityId id, ComponentTypeIndex type) const {
    const usize index = component_index(id, type);
    return index < _slots.size() ? _slots[component_index(id, type)].index : u32(-1);
}


usize ComponentMatrix::component_index(EntityId id, ComponentTypeIndex type) const {
    return id.index() * _type_count + usize(type);
}

}
}

