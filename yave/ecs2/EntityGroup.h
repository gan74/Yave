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

#include "ecs.h"
#include "traits.h"
#include "ComponentMutationTable.h"

#include <y/core/SlotVector.h>
#include <y/concurrent/Signal.h>

namespace yave {
namespace ecs2 {

class EntityGroupBase : NonMovable {
    public:
        virtual ~EntityGroupBase() = default;

        inline usize size() const {
            return _ids.size();
        }

        inline core::Span<ComponentTypeIndex> types() const {
            return _types;
        }

        inline core::Span<EntityId> ids() const {
            return _ids;
        }

    protected:
        friend class ComponentMatrix;

        virtual void add_entity(EntityId id, core::Span<u32> slots) = 0;
        virtual void remove_entity(EntityId id) = 0;

    protected:
        EntityGroupBase(core::Span<ComponentTypeIndex> types) : _types(types) {
        }

        core::Span<ComponentTypeIndex> _types;
        core::Vector<EntityId> _ids;
};

template<typename... Ts>
class EntityGroup final : public EntityGroupBase {
    static constexpr usize type_count = sizeof...(Ts);
    static constexpr usize mutating_count = ((traits::is_component_mutable_v<Ts> ? 1 : 0) + ...);
    static constexpr usize changed_count = ((traits::is_component_changed_v<Ts> ? 1 : 0) + ...);


    static inline const std::array<ComponentTypeIndex, type_count> type_storage = { type_index<traits::component_raw_type_t<Ts>>()... };



    template<typename T>
    using Slot = core::SlotVector<T>::Slot;

    using ContainerTuple = std::tuple<core::SlotVector<traits::component_raw_type_t<Ts>>*...>;
    using SlotTuple = std::tuple<Slot<traits::component_raw_type_t<Ts>>...>;
    using ComponentTuple = std::tuple<traits::component_type_t<Ts>&...>;
    using MutatingTableArray = std::array<ComponentMutationTable*, mutating_count>;
    using ChangedTableArray = std::array<ComponentMutationTable*, changed_count>;





    template<usize... Is>
    static inline SlotTuple make_slots(core::Span<u32> slots, std::index_sequence<Is...>) {
        y_debug_assert((slots[usize(type_storage[Is])] != u32(-1)) && ...);
        return SlotTuple{Slot<traits::component_raw_type_t<Ts>>(slots[usize(type_storage[Is])])...};
    }

    template<typename T>
    static inline T& get_component(const ContainerTuple& containers, const SlotTuple& slots) {
        using raw_type = traits::component_raw_type_t<T>;
        return (*std::get<core::SlotVector<raw_type>*>(containers))[std::get<Slot<raw_type>>(slots)];
    }





    template<typename T, usize I>
    static void fill_one(const std::array<ComponentMutationTable*, type_count>& tables,
                         MutatingTableArray& mutating, usize& mut_index,
                         ChangedTableArray& changed, usize& cha_index) {
        if constexpr(traits::is_component_mutable_v<T>) {
            mutating[mut_index++] = tables[I];
        }
        if constexpr(traits::is_component_changed_v<T>) {
            changed[cha_index++] = tables[I];
        }
    }

    template<usize... Is>
    static inline void fill_mutation_tables(const std::array<ComponentMutationTable*, type_count>& tables, MutatingTableArray& mutating, ChangedTableArray& changed, std::index_sequence<Is...>) {
        usize mut_index = 0;
        usize cha_index = 0;
        (fill_one<Ts, Is>(tables, mutating, mut_index, changed, cha_index), ...);
        y_debug_assert(mut_index == mutating_count);
        y_debug_assert(cha_index == changed_count);
    }

    static inline void update_mutation_tables(const MutatingTableArray& tables, EntityId id) {
        for(ComponentMutationTable* table : tables) {
            table->add(id);
        }
    }

    static inline bool matches_changed(const ChangedTableArray& tables, EntityId id) {
        return std::all_of(tables.begin(), tables.end(), [id](const auto* table) { return table->contains(id); });
    }





    class Iterator {
        public:
            using value_type = ComponentTuple;
            using size_type = usize;

            using reference = value_type;

            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;

            inline Iterator& operator++() {
                advance();
                return *this;
            }

            inline Iterator operator++(int) {
                const Iterator it = *this;
                advance();
                return it;
            }

            inline reference operator*() const {
                y_debug_assert(_index < _ids.size());
                update_mutation_tables(_mutating, _ids[_index]);
                const SlotTuple slots = _slots[_index];
                return ComponentTuple{EntityGroup::get_component<traits::component_type_t<Ts>>(_containers, slots)...};
            }

            inline std::strong_ordering operator<=>(const Iterator& other) const {
                return _index <=> other._index;
            }

            bool operator==(const Iterator&) const = default;
            bool operator!=(const Iterator&) const = default;

        private:
            friend class EntityGroup;

            Iterator(usize index, core::Span<EntityId> ids, core::Span<SlotTuple> slots, const ContainerTuple& containers, const MutatingTableArray& mutating, const ChangedTableArray& changed) :
                    _index(index),
                    _ids(ids),
                    _slots(slots),
                    _containers(containers),
                    _mutating(mutating),
                    _changed(changed) {
            }

            inline void advance() {
                for(++_index; _index != _ids.size(); ++_index) {
                    if(EntityGroup::matches_changed(_changed, _ids[_index])) {
                        break;
                    }
                }
            }

            usize _index = 0;
            core::Span<EntityId> _ids;
            core::Span<SlotTuple> _slots;
            ContainerTuple _containers = {};
            MutatingTableArray _mutating = {};
            ChangedTableArray _changed = {};
    };




    public:
        using const_iterator = Iterator;

        EntityGroup(ContainerTuple containers, const std::array<ComponentMutationTable*, type_count>& tables) : EntityGroupBase(type_storage), _containers(containers) {
            fill_mutation_tables(tables, _mutating_tables, _changed_tables, std::make_index_sequence<type_count>{});
        }

        const_iterator begin() const {
            return const_iterator(0, _ids, _component_slots, _containers, _mutating_tables, _changed_tables);
        }

        const_iterator end() const {
            return const_iterator(_ids.size(), _ids, _component_slots, _containers, _mutating_tables, _changed_tables);
        }

    protected:
        void add_entity(EntityId id, core::Span<u32> slots) {
            y_debug_assert(_component_slots.size() == _ids.size());
            _ids << id;
            _component_slots << make_slots(slots, std::make_index_sequence<type_count>{});

            _on_added.send(id);
        }

        void remove_entity(EntityId id) override {
            y_debug_assert(_component_slots.size() == _ids.size());
            _on_removed.send(id);

            const auto it = std::find(_ids.begin(), _ids.end(), id);
            y_debug_assert(it != _ids.end());

            const usize index = it - _ids.begin();
            _ids.erase_unordered(_ids.begin() + index);
            _component_slots.erase_unordered(_component_slots.begin() + index);
        }

    private:
        core::Vector<SlotTuple> _component_slots;
        ContainerTuple _containers;
        MutatingTableArray _mutating_tables = {};
        ChangedTableArray _changed_tables = {};

        concurrent::Signal<EntityId> _on_added;
        concurrent::Signal<EntityId> _on_removed;

};


}
}

#endif // YAVE_ECS2_ENTITYGROUP_H

