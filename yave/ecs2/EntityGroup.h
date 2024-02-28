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
#include "ComponentContainer.h"

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
            return _ids.ids();
        }

    protected:
        friend class ComponentMatrix;

        void add_entity(EntityId id) {
            _ids.insert(id);
        }

        void remove_entity(EntityId id) {
            _ids.erase(id);
        }


    protected:
        EntityGroupBase(core::Span<ComponentTypeIndex> types) : _types(types) {
        }

        core::Span<ComponentTypeIndex> _types;
        SparseIdSet _ids;
};


template<typename... Ts>
class EntityGroup final : public EntityGroupBase {
    static constexpr usize type_count = sizeof...(Ts);
    static constexpr usize mutate_count = ((traits::is_component_mutable_v<Ts> ? 1 : 0) + ...);
    static constexpr usize changed_count = ((traits::is_component_changed_v<Ts> ? 1 : 0) + ...);


    static inline const std::array<ComponentTypeIndex, type_count> type_storage = { type_index<traits::component_raw_type_t<Ts>>()... };


    using SetTuple = std::tuple<SparseComponentSet<traits::component_raw_type_t<Ts>>*...>;
    using ContainerTuple = std::tuple<ComponentContainer<traits::component_raw_type_t<Ts>>*...>;
    using ComponentTuple = std::tuple<traits::component_type_t<Ts>&...>;

    using MutateContainers = std::array<SparseIdSet*, mutate_count>;
    using ChangedContainers = std::array<const SparseIdSet*, changed_count>;


    template<typename T>
    static inline T& get_component(const SetTuple& sets, EntityId id) {
        return (*std::get<SparseComponentSet<traits::component_raw_type_t<T>>*>(sets))[id];
    }

    template<typename T, usize I>
    void fill_one(const ContainerTuple& containers, usize& mut_index, usize& cha_index) {
        std::get<I>(_sets) = &std::get<I>(containers)->_components;

        if constexpr(traits::is_component_mutable_v<T>) {
            _mutate[mut_index++] = &std::get<I>(containers)->_mutated;
        }
        if constexpr(traits::is_component_changed_v<T>) {
            _changed[cha_index++] = &std::get<I>(containers)->_mutated;
        }
    }

    template<usize... Is>
    inline void fill_sets(const ContainerTuple& containers, std::index_sequence<Is...>) {
        usize mut_index = 0;
        usize cha_index = 0;
        (fill_one<Ts, Is>(containers, mut_index, cha_index), ...);
        y_debug_assert(std::all_of(_mutate.begin(), _mutate.end(), [](const auto* s) { return s; }));
        y_debug_assert(std::all_of(_changed.begin(), _changed.end(), [](const auto* s) { return s; }));
    }



    class Iterator {
        public:
            using value_type = ComponentTuple;
            using size_type = usize;

            using reference = value_type;

            using iterator_category = std::forward_iterator_tag;
            using difference_type = std::ptrdiff_t;

            inline Iterator& operator++() {
                ++_it;
                return *this;
            }

            inline Iterator operator++(int) {
                const Iterator it = *this;
                ++_it;
                return it;
            }

            inline reference operator*() const {
                return ComponentTuple{EntityGroup::get_component<traits::component_type_t<Ts>>(_sets, *_it)...};
            }

            inline std::strong_ordering operator<=>(const Iterator& other) const {
                return _it <=> other._it;
            }

            bool operator==(const Iterator&) const = default;
            bool operator!=(const Iterator&) const = default;

        private:
            friend class EntityGroup;
            friend class Query;

            Iterator(const EntityId* it, const SetTuple& sets) : _it(it), _sets(sets) {
            }

            const EntityId* _it = nullptr;
            SetTuple _sets = {};
    };

    class Query {
        public:
            using const_iterator = Iterator;

            const_iterator begin() const {
                return const_iterator(ids().begin(), _sets);
            }

            const_iterator end() const {
                return const_iterator(ids().end(), _sets);
            }

            core::Span<EntityId> ids() const {
                return _ids.is_empty() ? core::Span<EntityId>(_owned) : _ids;
            }

        private:
            friend class EntityGroup;

            core::Span<EntityId> _ids;
            core::Vector<EntityId> _owned;
            SetTuple _sets = {};
    };

    public:
        EntityGroup(const ContainerTuple& containers) : EntityGroupBase(type_storage) {
            fill_sets(containers, std::make_index_sequence<type_count>{});
        }

        Query query() const {
            y_profile();

            Query query;
            query._sets = _sets;

            if constexpr(changed_count) {
                y_profile_zone("finding changed entities");

                std::array<const SparseIdSet*, changed_count + 1> matches = {};
                std::copy_n(_changed.begin(), changed_count, matches.begin());
                matches[changed_count] = &_ids;

                std::sort(matches.begin(), matches.end(), [](const SparseIdSet* a, const SparseIdSet* b) {
                    return a->size() < b->size();
                });

                for(const EntityId id : matches[0]->ids()) {
                    bool match = true;
                    for(usize i = 1; i != matches.size(); ++i) {
                        if(!matches[i]->contains(id)) {
                            match = false;
                            break;
                        }
                    }
                    if(match) {
                        query._owned << id;
                    }
                }
            } else {
                query._ids = _ids.ids();
            }

            if constexpr(mutate_count) {
                y_profile_zone("propagating mutations");
                for(SparseIdSet* mut_set : _mutate) {
                    for(const EntityId id : query.ids()) {
                        mut_set->insert(id);
                    }
                }
            }

            return query;
        }

    private:
        SetTuple _sets = {};

        MutateContainers _mutate = {};
        ChangedContainers _changed = {};

};


}
}

#endif // YAVE_ECS2_ENTITYGROUP_H

