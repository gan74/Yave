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

class EntityGroupBase final : NonMovable {

    template<typename... Ts>
    static core::Span<ComponentTypeIndex> type_storage() {
        static std::array<ComponentTypeIndex, sizeof...(Ts)> storage = { type_index<traits::component_raw_type_t<Ts>>()... };
        return storage;
    }

    template<typename T>
    static core::String clean_component_name() {
        return core::String(ct_type_name<traits::component_raw_type_t<T>>())
            .replaced("class ", "")
            .replaced("struct ", "")
            .replaced("yave::", "")
            .replaced("ecs::", "")
            .replaced("> ", ">")
        ;
    }

    template<typename... Ts>
    static core::String create_group_name(core::Span<std::string_view> tags) {
        core::String name = "EntityGroupBase<";
        name += ((clean_component_name<Ts>() + ", ") + ...);
        name.resize(name.size() - 2);
        name += ">";

        if(!tags.is_empty()) {
            name += "(";
            for(const std::string_view tag : tags) {
                name += "\"";
                name += tag;
                name += "\", ";
            }
            name.resize(name.size() - 2);
            name += ")";
        }

        return name;
    }

    public:
        EntityGroupBase(core::Span<ComponentTypeIndex> types, core::Span<std::string_view> tags, core::Span<ComponentTypeIndex> type_filters) :
                _types(types),
                _tags(tags.size()),
                _type_filters(type_filters),
                _component_count(u8(types.size() + tags.size() + type_filters.size())) {

            std::copy(tags.begin(), tags.end(), _tags.begin());
            y_always_assert(_component_count == types.size() + tags.size() + type_filters.size(), "Too many component types in group");
        }

        inline const core::String& name() const {
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

        inline const SparseIdSet& ids() const {
            return _ids;
        }

        template<typename... Ts>
        inline bool matches(core::Span<std::string_view> tags, core::Span<ComponentTypeIndex> filters) const {
            return _types == type_storage<Ts...>() &&
                _type_filters == filters &&
                (_tags.size() == tags.size() && std::equal(tags.begin(), tags.end(), _tags.begin()))
            ;
        }

    private:
        friend class ComponentMatrix;
        friend class EntityWorld;

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

        SparseIdSet _ids;
        core::Span<ComponentTypeIndex> _types;
        core::FixedArray<core::String> _tags;
        core::FixedArray<ComponentTypeIndex> _type_filters;

        core::Vector<u8> _entity_component_count;
        const u8 _component_count = 0;

        core::String _name = "Unnamed group base";
};





template<typename... Ts>
class EntityGroup final : NonCopyable {
    static constexpr usize type_count = sizeof...(Ts);
    static constexpr usize mutate_count = ((traits::is_component_mutable_v<Ts> ? 1 : 0) + ...);

    static constexpr usize changed_count = ((traits::is_component_changed_v<Ts> ? 1 : 0) + ...);
    static constexpr usize deleted_count = ((traits::is_component_deleted_v<Ts> ? 1 : 0) + ...);
    static constexpr usize filter_count = changed_count + deleted_count;


    using SetTuple = std::tuple<SparseComponentSet<traits::component_raw_type_t<Ts>>*...>;
    using ContainerTuple = std::tuple<ComponentContainer<traits::component_raw_type_t<Ts>>*...>;
    using ComponentTuple = std::tuple<traits::component_type_t<Ts>&...>;

    using MutateContainers = std::array<SparseIdSet*, mutate_count>;
    using FilterContainers = std::array<const SparseIdSet*, filter_count>;

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

    public:
        static constexpr bool is_const = !mutate_count;

        using const_iterator = Iterator<ComponentReturnPolicy>;


        ~EntityGroup() {
            if(_base) {
                unlock_all();
            }
        }

        // To avoid group being destroyed when used in ranged for (fixed in c++23)
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

        void swap(EntityGroup& other) {
            _ids.swap(other.ids());
            std::swap(_sets, other._sets);
            std::swap(_mutate, other._mutate);
            std::swap(_filter, other._filter);
            std::swap(_write_locks, other._write_locks);
            std::swap(_read_locks, other._read_locks);
            std::swap(_base, other._base);
        }

    private:
        friend class EntityWorld;

        EntityGroup(const EntityGroupBase* base, const ContainerTuple& containers) : _base(base) {
            fill_sets(containers, std::make_index_sequence<type_count>{});

            lock_all();

            if constexpr(filter_count) {
                y_profile_zone("finding changed entities");

                std::array<const SparseIdSet*, filter_count + 1> matches = {};
                std::copy_n(_filter.begin(), filter_count, matches.begin());
                matches[filter_count] = &_base->ids();

                std::sort(matches.begin(), matches.end(), [](const SparseIdSet* a, const SparseIdSet* b) {
                    return a->size() < b->size();
                });

                _ids.set_min_capacity(matches[0]->ids().size());

                for(const EntityId id : matches[0]->ids()) {
                    bool match = true;
                    for(usize i = 1; i != matches.size(); ++i) {
                        if(!matches[i]->contains(id)) {
                            match = false;
                            break;
                        }
                    }
                    if(match) {
                        _ids << id;
                    }
                }

                y_profile_msg(fmt_c_str("{} entities found", _ids.size()));
            } else {
                // This can be expensive. Find a way to avoid copying everything?
                _ids = _base->ids().ids();
            }


            if constexpr(mutate_count) {
                y_profile_dyn_zone(fmt_c_str("propagating mutation for {} entities", _ids.size()));
                for(SparseIdSet* mut_set : _mutate) {
                    for(const EntityId id : ids()) {
                        mut_set->insert(id);
                    }
                }
            }
        }

        void lock_all() {
            y_profile();

            usize write_index = 0;
            usize read_index = 0;

            auto unlock = [&] {
                for(usize i = 0; i != read_index; ++i) {
                    _read_locks[i]->unlock();
                }
                for(usize i = 0; i != write_index; ++i) {
                    _write_locks[i]->unlock();
                }

            };

            auto try_lock_all = [&] {
                for(; write_index != _write_locks.size(); ++write_index) {
                    if(!_write_locks[write_index]->try_lock()) {
                        return false;
                    }
                }
                for(; read_index != _read_locks.size(); ++read_index) {
                    if(!_read_locks[read_index]->try_lock_shared()) {
                        return false;
                    }
                }

                return true;
            };

            while(!try_lock_all()) {
                unlock();
                write_index = 0;
                read_index = 0;
            }
        }

        void unlock_all() {
            y_profile();

            for(auto* lock : _read_locks) {
                lock->unlock_shared();
            }

            for(auto* lock : _write_locks) {
                lock->unlock();
            }
        }



        core::Vector<EntityId> _ids;

        SetTuple _sets = {};

        MutateContainers _mutate = {};
        FilterContainers _filter = {};

        std::array<ComponentContainerBase::lock_type*, mutate_count> _write_locks = {};
        std::array<ComponentContainerBase::lock_type*, type_count - mutate_count> _read_locks = {};

        const EntityGroupBase* _base = nullptr;
};


}
}

#endif // YAVE_ECS_ENTITYGROUP_H

