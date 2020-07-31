/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

    public:
        class IDComponents {
            public:
                auto id() const {
                    return _id;
                }

                auto components() const {
                    return _components;
                }

                template<typename T>
                auto&& component() const {
                    using type = std::conditional_t<Const, const remove_cvref_t<T>&, remove_cvref_t<T>>;
                    constexpr usize index = detail::tuple_index<type, decltype(components())>::value;
                    static_assert(std::is_same_v<type, std::tuple_element_t<index, decltype(components())>>);
                    return std::get<index>(components());
                }

                IDComponents(EntityId id, component_tuple components) : _id(id), _components(components) {
                }

            private:
                EntityId _id;
                component_tuple _components;

        };


        EntityView(const set_tuple& sets) : _sets(sets), _short(shortest_range()) {
        }

        auto id_components() const {
            auto tr = [sets = _sets](EntityId id){ return IDComponents(id, make_component_tuple(sets, id)); };
            return core::Range(TransformIterator(ids().begin(), tr), ids().end());
        }

        auto components() const {
            auto tr = [sets = _sets](EntityId id) { return make_component_tuple(sets, id); };
            return core::Range(TransformIterator(ids().begin(), tr), ids().end());
        }

        auto ids() const {
            auto filter = [sets = _sets](EntityId id) { return has_all<0>(sets, id); };
            return core::Range(FilterIterator(_short.begin(), _short.end(), filter), EndIterator{});
        }


        auto begin() const {
            return id_components().begin();
        }

        auto end() const {
            return id_components().end();
        }


    private:
        set_tuple _sets;
        id_range _short;

    public:
        using const_iterator = decltype(std::declval<const EntityView<Const, Args...>>().begin());
};

}
}

#endif // YAVE_ECS_ENTITYVIEW_H

