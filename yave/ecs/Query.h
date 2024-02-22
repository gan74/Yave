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
#ifndef YAVE_ECS_QUERY_H
#define YAVE_ECS_QUERY_H

#include "traits.h"
#include "ComponentContainer.h"

#include <y/utils/iter.h>

#include <y/utils/log.h>

#define USE_LAZY_QUERY

namespace yave {
namespace ecs {

namespace detail {
template<typename T, typename U, typename... Args>
static constexpr usize tuple_index(const std::tuple<U, Args...>*) {
    if constexpr(std::is_same_v<T, std::remove_cvref_t<U>>) {
        return 0;
    } else {
        const std::tuple<Args...>* p = nullptr;
        return tuple_index<T>(p) + 1;
    }
}
}


struct QueryUtils {
    struct SetMatch {
        const SparseIdSetBase* set = nullptr;
        bool include = true;

        inline core::Span<EntityId> ids() const {
            return set ? set->ids() : core::Span<EntityId>{};
        }

        inline isize signed_size() const {
            return set ? set->size() : 0;
        }

        inline usize sorting_key() const {
            return usize(include ? signed_size() : -(signed_size() + 1));
        }

        inline bool is_empty() const {
            if(!include) {
                return false;
            }
            return set ? set->is_empty() : true;
        }

        inline bool contains(EntityId id) const {
            return (set ? set->contains(id) : false) == include;
        }
    };

    static core::Vector<EntityId> matching(core::Span<SetMatch> matches, core::Span<EntityId> ids);

    template<usize I = 0, typename... Args>
    static void fill_match_array(core::MutableSpan<SetMatch> matches, const std::array<const ComponentContainerBase*, sizeof...(Args)>& containers, bool only_changed = true) {
        if constexpr(I < sizeof...(Args)) {
            using component_type = std::tuple_element_t<I, std::tuple<Args...>>;
            static constexpr bool required = traits::component_required_v<component_type>;
            static constexpr bool changed = traits::component_changed_v<component_type>;

            const SparseIdSetBase* set = nullptr;
            if(const ComponentContainerBase* container = containers[I]) {

                y_debug_assert(type_index<traits::component_raw_type_t<component_type>>() == container->type_id());

                if(changed && only_changed) {
                    set = &container->recently_mutated();
                } else {
                    set = &container->id_set();
                }
            }

            matches[I] = {
                set,
                required,
            };
            fill_match_array<I + 1, Args...>(matches, containers);
        }
    }
};


template<typename... Args>
class Query : NonCopyable {

    using set_tuple = std::tuple<SparseComponentSet<traits::component_raw_type_t<Args>>*...>;
    using all_components = std::tuple<traits::component_type_t<Args>...>;

    // static constexpr bool tuple_is_empty = sizeof...(Args) == 0;
    static constexpr std::array component_included = {traits::component_required_v<Args>..., false};

    template<usize I = 0>
    static auto make_component_tuple(const set_tuple& sets, EntityId id) {
        if constexpr(I < std::tuple_size_v<set_tuple>) {
            if constexpr(!component_included[I]) {
                return make_component_tuple<I + 1>(sets, id);
            } else {
                y_debug_assert(std::get<I>(sets));
                auto&& s = *std::get<I>(sets);
                std::tuple<std::tuple_element_t<I, all_components>&> tpl = std::tie(s[id]);
                if constexpr(I + 1 == sizeof...(Args)) {
                    return tpl;
                } else {
                    return std::tuple_cat(tpl, make_component_tuple<I + 1>(sets, id));
                }
            }
        } else {
            return std::tie();
        }
    }
    public:
        using component_tuple = decltype(make_component_tuple(set_tuple{}, EntityId{}));

        struct IdComponents {
            EntityId id;
            component_tuple components;

            template<typename T>
            inline decltype(auto) component() const {
                constexpr component_tuple* p = nullptr;
                constexpr usize index = detail::tuple_index<T>(p);
                return std::get<index>(components);
            }

            inline IdComponents(EntityId i, component_tuple c) : id(i), components(c) {
            }

        };

    private:
        struct IdComponentsReturnPolicy {
            inline static decltype(auto) make(EntityId id, const component_tuple& comps) {
                return IdComponents(id, comps);
            }
        };

        struct ComponentsReturnPolicy {
            inline static component_tuple make(EntityId, const component_tuple& comps) {
                return comps;
            }
        };

        template<typename ReturnPolicy>
        class LazyIterator {
            public:
                inline LazyIterator() = default;

                inline LazyIterator& operator++() {
                    ++_it;
                    return *this;
                }

                inline LazyIterator operator++(int) {
                    const LazyIterator it = *this;
                    ++_it;
                    return it;
                }

                inline bool operator==(const LazyIterator& other) const {
                    return _it == other._it;
                }

                inline bool operator!=(const LazyIterator& other) const {
                    return _it != other._it;
                }

                inline auto operator*() const {
                    y_debug_assert(_it && _it->is_valid());
                    return ReturnPolicy::make(*_it, make_component_tuple(_sets, *_it));
                }

            private:
                friend class Query;

                inline LazyIterator(const EntityId* it, const set_tuple& sets) : _it(it), _sets(sets) {
                }

                const EntityId* _it = nullptr;
                set_tuple _sets;
        };

        template<typename ReturnPolicy>
        class VecIterator {
            public:
                inline VecIterator() = default;

                inline VecIterator& operator++() {
                    ++_index;
                    return *this;
                }

                inline VecIterator operator++(int) {
                    const VecIterator it = *this;
                    ++_index;
                    return it;
                }

                inline bool operator==(const VecIterator& other) const {
                    y_debug_assert(_ids == other._ids);
                    return _index == other._index;
                }

                inline bool operator!=(const VecIterator& other) const {
                    y_debug_assert(_ids == other._ids);
                    return _index != other._index;
                }

                inline auto operator*() const {
                    return ReturnPolicy::make(_ids[_index], _components[_index]);
                }

            private:
                friend class Query;

                inline VecIterator(usize index, const EntityId* ids, const component_tuple* components) : _index(index), _ids(ids), _components(components){
                }

                usize _index = 0;
                const EntityId* _ids = nullptr;
                const component_tuple* _components = nullptr;

        };

    public:
#if defined(USE_LAZY_QUERY)
        using const_iterator = LazyIterator<IdComponentsReturnPolicy>;
        using const_component_iterator = LazyIterator<ComponentsReturnPolicy>;

        const_iterator begin() const {
            return const_iterator(_ids.begin(), _sets);
        }

        const_iterator end() const {
            return const_iterator(_ids.end(), _sets);
        }

        const_component_iterator components_begin() const {
            return const_component_iterator(_ids.begin(), _sets);
        }

        const_component_iterator components_end() const {
            return const_component_iterator(_ids.end(), _sets);
        }
#else
        using const_iterator = VecIterator<IdComponentsReturnPolicy>;
        using const_component_iterator = typename core::Vector<component_tuple>::const_iterator;

        const_iterator begin() const {
            return const_iterator(0, _ids.data(), _components.data());
        }

        const_iterator end() const {
            return const_iterator(_ids.size(), _ids.data(), _components.data());
        }

        const_component_iterator components_begin() const {
            return _components.begin();
        }

        const_component_iterator components_end() const {
            return _components.end();
        }
#endif



        usize size() const {
            return _ids.size();
        }

        bool is_empty() const {
            return _ids.is_empty();
        }

        // These have lifetime problems when writing:
        // for(auto id : world.query<A>().ids()) { /* ... */ }
        // "world.query<A>().ids()" is what gets bound, so the Query gets destroyed before the loop is even entered...
        // We can kinda work around this using ref-qualifiers to make sure the Query is an lvalue.
        // const& doesn't work here sadly (because it makes query<A>().ids() valid again)
        core::Range<const_iterator> id_components() & {
            return {begin(), end()};
        }

        core::Range<const_component_iterator> components() & {
            return {components_begin(), components_end()};
        }

        core::Span<EntityId> ids() & {
            return _ids;
        }

        core::Vector<EntityId> to_ids() && {
            return std::move(_ids);
        }

    private:
        friend class EntityWorld;

        Query(const set_tuple& sets, core::MutableSpan<QueryUtils::SetMatch> matches) : _sets(sets) {
            if(!matches.is_empty() && std::all_of(matches.begin(), matches.end(), [](auto match) { return !match.is_empty(); })) {
                std::sort(matches.begin(), matches.end(), [](const auto& a, const auto& b) { return a.sorting_key() < b.sorting_key(); });
                y_always_assert(matches[0].include, "Query needs at least one inclusive matching rule");
                _ids = QueryUtils::matching(core::Span<QueryUtils::SetMatch>(matches.begin() + 1, matches.size() - 1), matches[0].ids());
            }
            fill_components_array();
        }

        Query(const set_tuple& sets, core::MutableSpan<QueryUtils::SetMatch> matches, core::Span<EntityId> range)  : _sets(sets) {
            if(std::all_of(matches.begin(), matches.end(), [](auto match) { return !match.is_empty(); })) {
                std::sort(matches.begin(), matches.end(), [](const auto& a, const auto& b) { return a.sorting_key() < b.sorting_key(); });
                _ids = QueryUtils::matching(matches, range);
            }
            fill_components_array();
        }

    private:
#if defined(USE_LAZY_QUERY)
        void fill_components_array() {}
#else
        void fill_components_array() {
            y_profile();
            _components.set_min_capacity(_ids.size());

            for(const EntityId id : _ids) {
                _components.emplace_back(make_component_tuple(_sets, id));
            }
        }

        core::Vector<component_tuple> _components;
#endif

        set_tuple _sets;

        core::Vector<EntityId> _ids;


};

}
}



#endif // YAVE_ECS_QUERY_H

