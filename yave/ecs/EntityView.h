/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#ifndef YAVE_ECS_ENTITYVIEW_H
#define YAVE_ECS_ENTITYVIEW_H

#include "SparseComponentSet.h"

#include <y/utils/iter.h>

namespace yave {
namespace ecs {

namespace detail {
// https://stackoverflow.com/questions/18063451/get-index-of-a-tuple-elements-type
template<typename T, typename Tpl>
struct tuple_index {
};

template<typename T, typename... Args>
struct tuple_index<T, std::tuple<T, Args...>> {
    static constexpr usize value = 0;
};

template<typename T, typename U, typename... Args>
struct tuple_index<T, std::tuple<U, Args...>> {
    static constexpr usize value = tuple_index<T, std::tuple<Args...>>::value + 1;
};
}


template<bool Const, typename... Args>
class EntityView {

    using set_tuple = std::conditional_t<Const,
                std::tuple<const SparseComponentSet<Args>*...>,
                std::tuple<SparseComponentSet<Args>*...>>;

    using component_tuple = std::conditional_t<Const,
                std::tuple<const Args&...>,
                std::tuple<Args&...>>;

    using id_range = core::Span<EntityId>;

    template<usize I = 0>
    static auto make_component_tuple(const set_tuple& sets, EntityId id) {
        y_debug_assert(std::get<I>(sets));
        auto&& s = *std::get<I>(sets);
        if constexpr(I + 1 == sizeof...(Args)) {
            return std::tie(s[id]);
        } else {
            return std::tuple_cat(std::tie(s[id]),
                                  make_component_tuple<I + 1>(sets, id));
        }
    }

    template<usize I = 0>
    id_range shortest_range() {
        const auto* s = std::get<I>(_sets);
        if(!s) {
            return id_range();
        }

        if constexpr(I + 1 == sizeof...(Args)) {
            return s->ids();
        } else {
            id_range shortest = shortest_range<I + 1>();
            return shortest.size() < s->size() ? shortest : s->ids();
        }
    }

    template<usize I = 0>
    static bool has_all(const set_tuple& sets, EntityId id) {
        if constexpr(I < sizeof...(Args)) {
            const auto* s = std::get<I>(sets);
            y_debug_assert(s);
            return s->contains_index(id.index()) && has_all<I + 1>(sets, id);
        }
        return true;
    }


    template<usize I = 0>
    [[maybe_unused]] static bool has_null_sets(const set_tuple& sets) {
        if constexpr(I < sizeof...(Args)) {
            if(!std::get<I>(sets)) {
                return true;
            }
            return has_null_sets<I + 1>(sets);
        }
            return false;
    }

    public:
        class IdComponents {
            public:
                inline auto id() const {
                    return _id;
                }

                inline auto components() const {
                    return _components;
                }

                template<typename T>
                inline decltype(auto) component() const {
                    using type = std::conditional_t<Const, const remove_cvref_t<T>&, remove_cvref_t<T>&>;
                    constexpr usize index = detail::tuple_index<type, decltype(components())>::value;
                    static_assert(std::is_same_v<type, std::tuple_element_t<index, decltype(components())>>);
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
            inline static decltype(auto) make(EntityId id, const set_tuple& sets) {
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
                friend class EntityView;

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
                        if(has_all<0>(_sets, _range[0])) {
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


        EntityView(const set_tuple& sets) : _sets(sets), _short(shortest_range()) {
            y_debug_assert(_short.is_empty() || !has_null_sets<0>(_sets));
        }

        EntityView(const set_tuple& sets, id_range ids) : _sets(sets) {
            if(!has_null_sets<0>(_sets)) {
                /*for(usize i = 0; i != ids.size(); ++i) {
                    if(has_all<0>(_sets, ids[i])) {
                        _short = id_range(ids.data() + i, ids.size() - i);
                        break;
                    }
                }*/
                _short = ids;
            }

            y_debug_assert(_short.is_empty() || !has_null_sets<0>(_sets));
        }


        core::Range<const_iterator, const_end_iterator> id_components() const {
            return {const_iterator(_short, _sets), const_end_iterator{}};
        }

        core::Range<const_component_iterator, const_end_iterator> components() const {
            return {const_component_iterator(_short, _sets), const_end_iterator{}};
        }

        core::Range<const_id_iterator, const_end_iterator> ids() const {
            return {const_id_iterator(_short, _sets), const_end_iterator{}};
        }

        const_iterator begin() const {
            return id_components().begin();
        }

        const_end_iterator end() const {
            return id_components().end();
        }

    private:
        set_tuple _sets;
        id_range _short;
};

}
}

#endif // YAVE_ECS_ENTITYVIEW_H

