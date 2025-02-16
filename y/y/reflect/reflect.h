/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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
#include <y/utils/hash.h>
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
    const u32 name_hash;

    inline constexpr NamedObject(T& t, std::string_view n, u32 h) : object(t), name(n), name_hash(h) {
    }

    inline constexpr NamedObject<const T> make_const_ref() const {
        return NamedObject<const T>(object, name, name_hash);
    }
};

template<typename T, typename M>
struct NamedMember {
    M T::* member;
    const std::string_view name;
    const u32 name_hash;

    inline constexpr NamedMember(M T::* m, std::string_view n, u32 h) : member(m), name(n), name_hash(h) {
    }

    inline constexpr const M& get(const T& obj) const {
        return obj.*member;
    }

    inline constexpr M& get(T& obj) const {
        return obj.*member;
    }

    inline constexpr NamedObject<M> materialize(T& obj) const {
        return NamedObject<M>(obj.*member, name, name_hash);
    }

    inline constexpr NamedObject<const M> materialize(const T& obj) const {
        return NamedObject<const M>(obj.*member, name, name_hash);
    }
};

namespace detail {

template<usize I, typename Tpl, typename F>
inline constexpr void explore_member(Tpl&& tpl, F&& func) {
    if constexpr(I < std::tuple_size_v<std::decay_t<Tpl>>) {
        auto&& elem = std::get<I>(tpl);
        func(elem.name, elem.member);
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

template<usize I, typename T, typename Tpl, typename F>
inline void explore_named_tuple(T&& t, Tpl&& tpl, F&& func) {
    if constexpr(I < std::tuple_size_v<std::decay_t<Tpl>>) {
        explore_one(std::get<I>(tpl).get(t), func);
        explore_named_tuple<I + 1>(t, tpl, func);
    }
}

template<typename T, typename F>
void explore_one(T&& t, F&& func) {
    if constexpr(has_reflect_v<T>) {
        auto elems = t._y_reflect_static();
        explore_named_tuple<0>(t, elems, func);
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
inline constexpr void explore_members(F&& func) {
    if constexpr(has_reflect_v<T>) {
        constexpr auto elems = T::_y_reflect_static();
        detail::explore_member<0>(elems, func);
    }
}

template<typename T>
inline consteval auto list_members() {
    if constexpr(has_reflect_v<T>) {
        return T::_y_reflect_static();
    } else {
        return std::tuple<>{};
    }
}

template<typename T>
inline consteval usize member_count() {
    return std::tuple_size_v<decltype(list_members<T>())>;
}


Y_TODO(manage inherited objects)
// https://godbolt.org/z/b6j3Ts

#define y_reflect_name_hash(name) y::force_ct<y::ct_str_hash(name)>()
#define y_reflect_create_member(member) y::reflect::NamedMember{&_y_refl_self_type::member, #member,  y_reflect_name_hash(#member)},

#define y_reflect_base(Type)                                                                                \
static constexpr std::string_view _y_reflect_type_name = #Type;                                             \

#define y_reflect_empty(Type)                                                                               \
y_reflect_base(Type)                                                                                        \
template<typename = void> static inline consteval auto _y_reflect_static() const { return std::tuple<>{}; }

#define y_reflect_static(Type, ...)                                                                         \
y_reflect_base(Type);                                                                                       \
template<typename = void> static inline consteval auto _y_reflect_static() {                                \
    using _y_refl_self_type = Type;                                                                         \
    return std::tuple{Y_REC_MACRO(Y_MACRO_MAP(y_reflect_create_member, __VA_ARGS__))};                      \
}


#define y_reflect(Type, ...)    y_reflect_static(Type, __VA_ARGS__)


}
}

#endif // Y_REFLECT_REFLECT_H

