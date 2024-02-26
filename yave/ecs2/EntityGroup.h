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
#ifndef YAVE_ECS2_ENTITYGROUP_H
#define YAVE_ECS2_ENTITYGROUP_H

#include "ComponentContainer.h"

namespace yave {
namespace ecs2 {

class EntityGroupBase : NonMovable {
    public:
        virtual ~EntityGroupBase() = default;

        core::Span<ComponentTypeIndex> types() const {
            return _types;
        }

        core::Span<EntityId> ids() const {
            return _ids;
        }

    protected:
        friend class ComponentMatrix;

        virtual void add_entity(EntityId id, core::Span<u32> slots) = 0;
        virtual bool remove_entity(EntityId id) = 0;

    protected:
        EntityGroupBase(core::Span<ComponentTypeIndex> types) : _types(types) {
        }

        core::Span<ComponentTypeIndex> _types;
        core::Vector<EntityId> _ids;
};

template<typename... Ts>
class EntityGroup final : public EntityGroupBase {
    static constexpr usize type_count = sizeof...(Ts);

    template<typename T>
    using Slot = core::SlotVector<T>::Slot;

    using ContainerTuple = std::tuple<ComponentContainer<Ts>*...>;
    using SlotTuple = std::tuple<Slot<Ts>...>;

    static SlotTuple make_slots(core::Span<u32> slots) {
        return SlotTuple{Slot<Ts>(slots[usize(type_index<Ts>())])...};
    }

    static inline const std::array<ComponentTypeIndex, type_count> type_storage = { type_index<Ts>()... };

    public:
        EntityGroup(ContainerTuple containers) : EntityGroupBase(type_storage), _containers(containers) {
        }

    protected:
        void add_entity(EntityId id, core::Span<u32> slots) {
            y_debug_assert(_component_slots.size() == _ids.size());
            _ids << id;
            _component_slots << make_slots(slots);
        }

        bool remove_entity(EntityId id) override {
            y_debug_assert(_component_slots.size() == _ids.size());
            y_fatal("oof");
        }

    private:
        core::Vector<SlotTuple> _component_slots;
        ContainerTuple _containers;
};


}
}

#endif // YAVE_ECS2_ENTITYGROUP_H

