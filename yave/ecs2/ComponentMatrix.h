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
#ifndef YAVE_ECS2_COMPONENTMATRIX_H
#define YAVE_ECS2_COMPONENTMATRIX_H

#include "ecs.h"

#include <y/core/Vector.h>
#include <y/core/SlotVector.h>

namespace yave {
namespace ecs2 {

class ComponentMatrix {
    template<typename T>
    using TypedSlot = core::SlotVector<T>::Slot;

    public:
        void add_entity(EntityId id);
        void remove_entity(EntityId id);

        bool has_component(EntityId id, ComponentTypeIndex type) const;

        bool contains(EntityId id) const;

        template<typename T>
        void has_component(EntityId id) const {
            has_component(id, type_index<T>());
        }

        template<typename T>
        void add_component(EntityId id, TypedSlot<T> slot) {
            add_component(id, type_index<T>(), u32(slot));
        }

        template<typename T>
        void remove_component(EntityId id, TypedSlot<T> slot) {
            remove_component(id, type_index<T>(), u32(slot));
        }

        template<typename T>
        TypedSlot<T> component_slot_index(EntityId id) const {
            return TypedSlot<T>(component_slot_index(id, type_index<T>()));
        }


    private:
        friend class EntityWorld;

        ComponentMatrix(usize type_count);

        void register_group(EntityGroupBase* group);
        bool in_group(EntityId id, const EntityGroupBase* group) const;

        void add_component(EntityId id, ComponentTypeIndex type, u32 slot_index);
        void remove_component(EntityId id, ComponentTypeIndex type);

        u32 component_slot_index(EntityId id, ComponentTypeIndex type) const;
        usize component_index(EntityId id, ComponentTypeIndex type) const;
        core::Span<u32> slots_for_entity(EntityId id) const;

        usize _type_count = 0;
        core::Vector<u32> _slots;
        core::Vector<EntityId> _ids;

        core::FixedArray<core::Vector<EntityGroupBase*>> _groups;
};


}
}

#endif // YAVE_ECS2_COMPONENTMATRIX_H

