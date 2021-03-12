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
#ifndef Y_REFLECT_REFLECT_H
#define Y_REFLECT_REFLECT_H

#include "traits.h"

#include <y/utils.h>
#include <y/utils/recmacros.h>

#include <tuple>
#include <string_view>

#include <tuple>

namespace y {
namespace reflect {

template<typename T>
struct NamedObject {
    T& object;
    const std::string_view name;

    inline constexpr NamedObject(T& t, std::string_view n) : object(t), name(n) {
    }

    inline constexpr NamedObject<const T> make_const_ref() const {
        return NamedObject<const T>(object, name);
    }
};

namespace detail {

template<usize I, typename Tpl, typename F>
inline void explore_member(Tpl&& tpl, F&& func) {
    if constexpr(I < std::tuple_size_v<std::decay_t<Tpl>>) {
        auto&& elem = std::get<I>(tpl);
        func(elem.name, elem.object);
        explore_member<I + 1>(tpl, func);
    }
}


template<typename T, typename F>
void explore_one(T&& t, F&& func);

template<typename C, typename F>
inline void explore_collection(C&& col, F&& func) {
    for(auto&& item : col) {
        explore_one(item, func);
    }
}

template<usize I, typename Tpl, typename F>
inline void explore_tuple(Tpl&& tpl, F&& func) {
    if constexpr(I < std::tuple_size_v<std::decay_t<Tpl>>) {
        explore_one(std::get<I>(tpl), func);
        explore_tuple<I + 1>(tpl, func);
    }
}

template<usize I, typename Tpl, typename F>
inline void explore_named_tuple(Tpl&& tpl, F&& func) {
    if constexpr(I < std::tuple_size_v<std::decay_t<Tpl>>) {
        explore_one(std::get<I>(tpl).object, func);
        explore_named_tuple<I + 1>(tpl, func);
    }
}

template<typename T, typename F>
void explore_one(T&& t, F&& func) {
    if constexpr(has_reflect_v<T>) {
        auto elems = t._y_reflect();
        explore_named_tuple<0>(elems, func);
    } else if constexpr(is_tuple_v<T>) {
        explore_tuple<0>(t);
    } else if constexpr(is_std_ptr_v<T> || std::is_pointer_v<T>) {
        if(t != nullptr) {
            explore_one(*t, func);
        }
    } else if constexpr(is_iterable_v<T>) {
        explore_collection(t, func);
    }
    func(t);
}
}


template<typename T, typename F>
inline void explore_recursive(T&& t, F&& func) {
    detail::explore_one(t, func);
}


template<typename T, typename F>
inline void explore_members(T&& t, F&& func) {
    if constexpr(has_reflect_v<T>) {
        auto elems = t._y_reflect();
        detail::explore_member<0>(elems, func);
    }
}


Y_TODO(manage inherited objects)
// https://godbolt.org/z/b6j3Ts

#define y_reflect_create_item(object) y::reflect::NamedObject{object, #object},

#define y_reflect_refl_qual(qual, ...) template<typename = void> auto _y_reflect() qual { return std::tuple{Y_REC_MACRO(Y_MACRO_MAP(y_reflect_create_item, __VA_ARGS__))}; }

#define y_reflect_empty() template<typename = void> auto _y_reflect() const { return std::tuple<>{}; }

#define y_reflect(...)                              \
    y_reflect_refl_qual(/* */, __VA_ARGS__)         \
    y_reflect_refl_qual(const, __VA_ARGS__)


}
}

#endif // Y_SERDE3_REFLECT_H

