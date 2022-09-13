/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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
#include "SparseComponentSet.h"

#include <y/utils/iter.h>

namespace yave {
namespace ecs {

namespace detail {
template<typename T, typename U, typename... Args>
static constexpr usize tuple_index(const std::tuple<U, Args...>*) {
    if constexpr(std::is_same_v<T, remove_cvref_t<U>>) {
        return 0;
    } else {
        const std::tuple<Args...>* p = nullptr;
        return tuple_index<T>(p) + 1;
    }
}
}


struct QueryUtils {
    static core::Vector<EntityId> matching(core::Span<const SparseIdSetBase*> sets, core::Span<EntityId> ids);

    template<usize I = 0, typename T>
    static void fill_set_ids(core::MutableSpan<const SparseIdSetBase*> ids, const T& sets) {
        if constexpr(I < std::tuple_size_v<T>) {
            ids[I] = std::get<I>(sets);
            fill_set_ids<I + 1>(ids, sets);
        }
    }

    template<typename T>
    static auto create_set_array(const T& sets) {
        std::array<const SparseIdSetBase*, std::tuple_size_v<T>> ids = {};
        fill_set_ids(ids, sets);
        return ids;
    }
};


template<typename... Args>
class Query : NonCopyable {

    using set_tuple = std::tuple<SparseComponentSet<traits::component_raw_type_t<Args>>*...>;
    using all_components = std::tuple<traits::component_type_t<Args>...>;

    static constexpr bool is_empty = sizeof...(Args) == 0;
    static constexpr std::array component_required = {traits::component_required_v<Args>..., false};

    template<usize I = 0>
    static auto make_component_tuple(const set_tuple& sets, EntityId id) {
        if constexpr(!component_required[I]) {
            return std::tie();
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
    }
    public:
        using component_tuple = decltype(make_component_tuple(set_tuple{}, EntityId{}));

        class IdComponents {
            public:
                inline auto id() const {
                    return _id;
                }

                inline component_tuple components() const {
                    return _components;
                }

                template<typename T>
                inline decltype(auto) component() const {
                    constexpr component_tuple* p = nullptr;
                    constexpr usize index = detail::tuple_index<T>(p);
                    return std::get<index>(components());
                }

                inline IdComponents(EntityId id, component_tuple components) : _id(id), _components(components) {
                }

            private:
                EntityId _id;
                component_tuple _components;
        };

    private:
        struct IdComponentsReturnPolicy {
            inline static decltype(auto) make(EntityId id, const set_tuple& sets) {
                return IdComponents(id, make_component_tuple(sets, id));
            }
        };

        struct ComponentsReturnPolicy {
            inline static component_tuple make(EntityId id, const set_tuple& sets) {
                return make_component_tuple(sets, id);
            }
        };

        template<typename ReturnPolicy>
        class Iterator {
            public:
                inline Iterator() = default;

                inline Iterator& operator++() {
                    ++_it;
                    return *this;
                }

                inline Iterator operator++(int) {
                    const Iterator it = *this;
                    ++_it;
                    return it;
                }

                inline bool operator==(const Iterator& other) const {
                    return _it == other._it;
                }

                inline bool operator!=(const Iterator& other) const {
                    return _it != other._it;
                }

                inline auto operator*() const {
                    y_debug_assert(_it && _it->is_valid());
                    return ReturnPolicy::make(*_it, _sets);
                }

            private:
                friend class Query;

                inline Iterator(const EntityId* it, const set_tuple& sets) : _it(it), _sets(sets) {
                }

                const EntityId* _it = nullptr;
                set_tuple _sets;
        };


    public:
        using const_iterator = Iterator<IdComponentsReturnPolicy>;

        using const_component_iterator = Iterator<ComponentsReturnPolicy>;

        const_iterator begin() const {
            return const_iterator(_ids.begin(), _sets);
        }

        const_iterator end() const {
            return const_iterator(_ids.end(), _sets);
        }

        usize size() const {
            return _ids.size();
        }

        // These have lifetime problems when writing:
        // for(auto id : world.query<A>().ids()) { /* ... */ }
        // "world.query<A>().ids()" is what gets bound, so the Query gets destroyed before the loop is even entered...
        // We can kinda work around this using ref-qualifiers to make sure the Query is an lvalue.
        // const& doesn't work here sadly (because it makes query<A>().ids() valid again)
        core::Range<const_iterator> id_components() & {
            return {const_iterator(_ids.begin(), _sets), const_iterator(_ids.end(), _sets)};
        }

        core::Range<const_component_iterator> components() & {
            return {const_component_iterator(_ids.begin(), _sets), const_component_iterator(_ids.end(), _sets)};
        }

        core::Span<EntityId> ids() & {
            return _ids;
        }

        core::Vector<EntityId> ids() && {
            return std::move(_ids);
        }

    private:
        friend class EntityWorld;

        Query(const set_tuple& sets, core::MutableSpan<const SparseIdSetBase*> id_sets) : _sets(sets) {
            if(!id_sets.is_empty() && std::all_of(id_sets.begin(), id_sets.end(), [](auto ptr) { return !!ptr; })) {
                std::sort(id_sets.begin(), id_sets.end(), [](const auto& a, const auto& b) { return a->size() < b->size(); });
                _ids = QueryUtils::matching(core::Span<const SparseIdSetBase*>(id_sets.begin() + 1, id_sets.size() - 1), id_sets[0]->ids());
            }
        }

        Query(const set_tuple& sets, core::MutableSpan<const SparseIdSetBase*> id_sets, core::Span<EntityId> range)  : _sets(sets) {
            if(std::all_of(id_sets.begin(), id_sets.end(), [](auto ptr) { return !!ptr; })) {
                std::sort(id_sets.begin(), id_sets.end(), [](const auto& a, const auto& b) { return a->size() < b->size(); });
                _ids = QueryUtils::matching(id_sets, range);
            }
        }

        Query(const set_tuple& sets) : Query(sets, QueryUtils::create_set_array(sets)) {
        }

        Query(const set_tuple& sets, core::Span<EntityId> range) : Query(sets, QueryUtils::create_set_array(sets), range) {
        }

    private:
        set_tuple _sets;

        core::Vector<EntityId> _ids;


};

}
}

#endif // YAVE_ECS_QUERY_H

