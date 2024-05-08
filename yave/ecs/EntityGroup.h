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
#ifndef YAVE_ECS_ENTITYGROUP_H
#define YAVE_ECS_ENTITYGROUP_H

#include "ecs.h"
#include "traits.h"
#include "ComponentContainer.h"

#include <y/concurrent/Signal.h>
#include <y/core/String.h>

namespace yave {
namespace ecs {

class EntityGroupBase : NonMovable {
    public:
        virtual ~EntityGroupBase() = default;

        const core::String& name() const {
            return _name;
        }

        inline core::Span<ComponentTypeIndex> types() const {
            return _types;
        }
        inline core::Span<core::String> tags() const {
            return _tags;
        }

        inline core::Span<ComponentTypeIndex> type_filters() const {
            return _type_filters;
        }

        inline core::Span<EntityId> ids_before_filtering() const {
            return _ids.ids();
        }


        inline bool matches(core::Span<std::string_view> tags, core::Span<ComponentTypeIndex> filters) const {
            if(tags.size() != _tags.size()) {
                return false;
            }
            return _type_filters == filters && std::equal(tags.begin(), tags.end(), _tags.begin());
        }

    protected:
        friend class ComponentMatrix;

        void add_entity_component(EntityId id) {
            _entity_component_count.set_min_size(id.index() + 1);
            y_debug_assert(_entity_component_count[id.index()] < _component_count);
            if(++_entity_component_count[id.index()] == _component_count) {
                y_debug_assert(!_ids.contains(id));
                _ids.insert(id);
            }
        }

        void remove_entity_component(EntityId id) {
            y_debug_assert(_entity_component_count.size() > id.index());
            const u8 prev_count = _entity_component_count[id.index()]--;
            if(prev_count == _component_count) {
                y_debug_assert(_ids.contains(id));
                _ids.erase(id);
            }
        }

    protected:
        EntityGroupBase(core::Span<ComponentTypeIndex> types, core::Span<std::string_view> tags, core::Span<ComponentTypeIndex> type_filters) :
                _types(types),
                _tags(tags.size()),
                _type_filters(type_filters),
                _component_count(u8(types.size() + tags.size() + type_filters.size())) {
            std::copy(tags.begin(), tags.end(), _tags.begin());
            y_always_assert(_component_count == types.size() + tags.size() + type_filters.size(), "Too many component types in group");
        }

        SparseIdSet _ids;
        core::Span<ComponentTypeIndex> _types;
        core::FixedArray<core::String> _tags;
        core::FixedArray<ComponentTypeIndex> _type_filters;

        core::Vector<u8> _entity_component_count;
        const u8 _component_count = 0;

        core::String _name;
};


template<typename... Ts>
class EntityGroup final : public EntityGroupBase {
    static constexpr usize type_count = sizeof...(Ts);
    static constexpr usize mutate_count = ((traits::is_component_mutable_v<Ts> ? 1 : 0) + ...);

    static constexpr usize changed_count = ((traits::is_component_changed_v<Ts> ? 1 : 0) + ...);
    static constexpr usize deleted_count = ((traits::is_component_deleted_v<Ts> ? 1 : 0) + ...);
    static constexpr usize filter_count = changed_count + deleted_count;


    static inline const std::array<ComponentTypeIndex, type_count> type_storage = { type_index<traits::component_raw_type_t<Ts>>()... };


    using SetTuple = std::tuple<SparseComponentSet<traits::component_raw_type_t<Ts>>*...>;
    using ContainerTuple = std::tuple<ComponentContainer<traits::component_raw_type_t<Ts>>*...>;
    using ComponentTuple = std::tuple<traits::component_type_t<Ts>&...>;

    using MutateContainers = std::array<SparseIdSet*, mutate_count>;
    using FilterContainers = std::array<const SparseIdSet*, filter_count>;


    template<typename T>
    static core::String clean_component_name() {
        return core::String(ct_type_name<T>())
            .replaced("class ", "")
            .replaced("struct ", "")
            .replaced("yave::", "")
            .replaced("ecs::", "")
            .replaced("> ", ">")
        ;
    }


    template<typename T>
    static inline T& get_component(const SetTuple& sets, EntityId id) {
        return (*std::get<SparseComponentSet<traits::component_raw_type_t<T>>*>(sets))[id];
    }

    template<typename T, usize I>
    void fill_one(const ContainerTuple& containers, usize& mut_index, usize& filter_index, usize& const_index) {
        std::get<I>(_sets) = &std::get<I>(containers)->_components;

        if constexpr(traits::is_component_mutable_v<T>) {
            _write_locks[mut_index] = &std::get<I>(containers)->_lock;
            _mutate[mut_index] = &std::get<I>(containers)->_mutated;
            ++mut_index;
        } else {
            _read_locks[const_index++] = &std::get<I>(containers)->_lock;
        }

        if constexpr(traits::is_component_changed_v<T>) {
            _filter[filter_index++] = &std::get<I>(containers)->_mutated;
        }
        if constexpr(traits::is_component_deleted_v<T>) {
            _filter[filter_index++] = &std::get<I>(containers)->_to_delete;
        }
    }

    template<usize... Is>
    inline void fill_sets(const ContainerTuple& containers, std::index_sequence<Is...>) {
        usize mut_index = 0;
        usize filter_index = 0;
        usize const_index = 0;
        (fill_one<Ts, Is>(containers, mut_index, filter_index, const_index), ...);

        y_debug_assert(std::all_of(_mutate.begin(), _mutate.end(), [](const auto* s) { return s; }));
        y_debug_assert(std::all_of(_filter.begin(), _filter.end(), [](const auto* s) { return s; }));
        y_debug_assert(std::all_of(_write_locks.begin(), _write_locks.end(), [](const auto* s) { return s; }));
        y_debug_assert(std::all_of(_read_locks.begin(), _read_locks.end(), [](const auto* s) { return s; }));
    }


    struct ComponentReturnPolicy {
        using value_type = ComponentTuple;
        using reference = value_type;

        static inline reference make(EntityId id, const SetTuple& sets) {
            return value_type{EntityGroup::get_component<traits::component_type_t<Ts>>(sets, id)...};
        }
    };

    struct IdComponentReturnPolicy {
        using value_type = std::tuple<EntityId, traits::component_type_t<Ts>&...>;
        using reference = value_type;

        static inline reference make(EntityId id, const SetTuple& sets) {
            return value_type{id, EntityGroup::get_component<traits::component_type_t<Ts>>(sets, id)...};
        }
    };

    template<typename ReturnPolicy>
    class Iterator {
        public:
            using value_type = typename ReturnPolicy::value_type;
            using reference = typename ReturnPolicy::reference;

            using size_type = usize;

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

            inline auto operator*() const {
                return ReturnPolicy::make(*_it, _sets);
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

    class Query : NonCopyable {
        public:
            using const_iterator = Iterator<ComponentReturnPolicy>;

            Query() = default;

            Query(const EntityGroup* parent) : _parent(parent) {
                _parent->lock_groups();
            }

            ~Query() {
                if(_parent) {
                    _parent->unlock_groups();
                }
            }

            Query(Query&& other) {
                swap(other);
            }

            Query& operator=(Query&& other) {
                swap(other);
                return *this;
            }

            void swap(Query& other) {
                std::swap(_ids, other._ids);
                std::swap(_sets, other._sets);
                std::swap(_parent, other._parent);
            }

            inline auto id_components() & {
                return core::Range(
                    Iterator<IdComponentReturnPolicy>(ids().begin(), _sets),
                    Iterator<IdComponentReturnPolicy>(ids().end(), _sets)
                );
            }

            inline const_iterator begin() const {
                return const_iterator(_ids.begin(), _sets);
            }

            inline const_iterator end() const {
                return const_iterator(_ids.end(), _sets);
            }

            inline core::Span<EntityId> ids() const {
                return _ids;
            }

            inline usize size() const {
                return ids().size();
            }

            inline bool is_empty() const {
                return _ids.is_empty();
            }

        private:
            friend class EntityGroup;

            core::Vector<EntityId> _ids;
            SetTuple _sets = {};

            const EntityGroup* _parent = nullptr;
    };

    public:
        static constexpr bool is_const = !mutate_count;

        EntityGroup(const ContainerTuple& containers, core::Span<std::string_view> tags, core::Span<ComponentTypeIndex> type_filters = {}) : EntityGroupBase(type_storage, tags, type_filters) {
            fill_sets(containers, std::make_index_sequence<type_count>{});

            _name = "EntityGroup<";
            _name += ((clean_component_name<Ts>() + ", ") + ...);
            _name.resize(_name.size() - 2);
            _name += ">";
        }

        Query query() const {
            y_profile();

            Query query(this);
            query._sets = _sets;

            if constexpr(filter_count) {
                y_profile_zone("finding changed entities");

                std::array<const SparseIdSet*, filter_count + 1> matches = {};
                std::copy_n(_filter.begin(), filter_count, matches.begin());
                matches[filter_count] = &_ids;

                std::sort(matches.begin(), matches.end(), [](const SparseIdSet* a, const SparseIdSet* b) {
                    return a->size() < b->size();
                });

                query._ids.set_min_capacity(matches[0]->ids().size());

                for(const EntityId id : matches[0]->ids()) {
                    bool match = true;
                    for(usize i = 1; i != matches.size(); ++i) {
                        if(!matches[i]->contains(id)) {
                            match = false;
                            break;
                        }
                    }
                    if(match) {
                        query._ids << id;
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
        void lock_groups() const {
            y_profile();

            Y_TODO(This can deadlock if a group locks A exclusively and B shared while another group does the opposite)

            for(auto* lock : _write_locks) {
                lock->lock();
            }

            for(auto* lock : _read_locks) {
                lock->lock_shared();
            }
        }

        void unlock_groups() const {
            y_profile();

            for(auto* lock : _read_locks) {
                lock->unlock_shared();
            }

            for(auto* lock : _write_locks) {
                lock->unlock();
            }
        }

        SetTuple _sets = {};

        MutateContainers _mutate = {};
        FilterContainers _filter = {};

        std::array<std::shared_mutex*, mutate_count> _write_locks;
        std::array<std::shared_mutex*, type_count - mutate_count> _read_locks;


};


}
}

#endif // YAVE_ECS_ENTITYGROUP_H

