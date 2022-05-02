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


template<typename... Args>
class Query {

    using set_tuple = std::tuple<SparseComponentSet<traits::component_raw_type_t<Args>>*...>;
    using all_components = std::tuple<traits::component_type_t<Args>...>;

    static constexpr std::array component_required = {traits::component_required_v<Args>...};

    using id_range = core::Span<EntityId>;


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

    template<usize I = 0>
    id_range shortest_range(const set_tuple& sets) {
        const auto* s = std::get<I>(sets);
        if(!s) {
            return id_range();
        }

        if constexpr(I + 1 == sizeof...(Args)) {
            return s->ids();
        } else {
            const id_range a = shortest_range<I + 1>(sets);
            const id_range b = s->ids();
            return a.size() < b.size() ? a : b;
        }
    }

    template<usize I = 0>
    static bool matches(const set_tuple& sets, EntityId id) {
        if constexpr(I < sizeof...(Args)) {
            const auto* s = std::get<I>(sets);
            y_debug_assert(s);
            return (component_required[I] == s->contains_index(id.index())) && matches<I + 1>(sets, id);
        }
        return true;
    }

    template<usize I = 0>
    static bool has_null_sets(const set_tuple& sets) {
        if constexpr(I < sizeof...(Args)) {
            if(!std::get<I>(sets)) {
                return true;
            }
            return has_null_sets<I + 1>(sets);
        }
        return false;
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

        struct IdReturnPolicy {
            inline static EntityId make(EntityId id, const set_tuple&) {
                return id;
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

                inline bool at_end() const {
                    return _range.is_empty();
                }

                inline Iterator& operator++() {
                    advance();
                    return *this;
                }

                inline Iterator operator++(int) {
                    const Iterator it = *this;
                    advance();
                    return it;
                }

                inline bool operator==(const Iterator& other) const {
                    return _range == other._range;
                }

                inline bool operator!=(const Iterator& other) const {
                    return _range != other._range;
                }

                inline auto operator*() const {
                    return ReturnPolicy::make(_range[0], _sets);
                }

            private:
                friend class Query;

                inline Iterator(id_range range, const set_tuple& sets) : _range(range), _sets(sets) {
                    find_next_valid();
                }

                inline void advance() {
                    y_debug_assert(!at_end());
                    move_one();
                    find_next_valid();
                }

                inline void find_next_valid() {
                    while(!at_end()) {
                        if(matches(_sets, _range[0])) {
                            break;
                        }
                        move_one();
                    }
                }

                inline void move_one() {
                    y_debug_assert(!_range.is_empty());
                    _range = id_range(_range.begin() + 1, _range.size() - 1);
                }

                id_range _range;
                set_tuple _sets;
        };


    public:
        using const_iterator = Iterator<IdComponentsReturnPolicy>;

        using const_component_iterator = Iterator<ComponentsReturnPolicy>;
        using const_id_iterator = Iterator<IdReturnPolicy>;

        using const_end_iterator = EndIterator;


        Query(const set_tuple& sets) : Query(sets, shortest_range(sets)) {
        }

        Query(const set_tuple& sets, id_range ids) : _sets(sets), _shortest(ids) {
            if(has_null_sets(_sets)) {
                _shortest = {};
            }
        }


        core::Range<const_iterator, const_end_iterator> id_components() const {
            return {const_iterator(_shortest, _sets), const_end_iterator{}};
        }

        core::Range<const_component_iterator, const_end_iterator> components() const {
            return {const_component_iterator(_shortest, _sets), const_end_iterator{}};
        }

        core::Range<const_id_iterator, const_end_iterator> ids() const {
            return {const_id_iterator(_shortest, _sets), const_end_iterator{}};
        }

        const_iterator begin() const {
            return id_components().begin();
        }

        const_end_iterator end() const {
            return id_components().end();
        }


    private:
        set_tuple _sets;
        id_range _shortest;

};

}
}

#endif // YAVE_ECS_QUERY_H

