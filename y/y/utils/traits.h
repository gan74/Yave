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
#ifndef Y_UTILS_TRAITS_H
#define Y_UTILS_TRAITS_H

#include "types.h"

#include <type_traits>

namespace y {

static_assert(std::is_move_assignable_v<NonCopyable>);
static_assert(std::is_move_constructible_v<NonCopyable>);

static_assert(!std::is_move_assignable_v<NonMovable>);
static_assert(!std::is_move_constructible_v<NonMovable>);


template<typename T>
void do_not_destroy(T&& t) {
    union U {
        U() {}
        ~U() {}
        std::remove_reference_t<T> t;
    } u;
    ::new(&u.t) std::remove_reference_t<T>(y_fwd(t));
}

template<bool C, typename T>
using const_type_t = std::conditional_t<C, const T, T>;


// type traits
template<typename T, typename... Args>
struct function_traits : function_traits<decltype(&T::operator())> {};

template<typename Ret, typename... Args>
struct function_traits<Ret(*)(Args...)> : function_traits<Ret(Args...)> {};

template<typename Ret, typename... Args>
struct function_traits<Ret(&)(Args...)> : function_traits<Ret(Args...)> {};

template<typename T, typename Ret, typename... Args>
struct function_traits<Ret(T::*)(Args...)> : function_traits<Ret(Args...)> {};

template<typename T, typename Ret, typename... Args>
struct function_traits<Ret(T::*)(Args...) const> : function_traits<Ret(Args...)> {};

/*template<typename Ret, typename... Args>
struct function_traits<Ret(Args...) const> : function_traits<Ret(Args...)> {};*/

template<typename Ret, typename... Args>
struct function_traits<Ret(Args...)> {
    using return_type = Ret;
    using func_type = Ret(Args...);

    static constexpr usize arg_count = sizeof...(Args);

    using argument_pack = std::tuple<std::remove_const_t<Args>...>;

    template<usize I>
    struct args {
        using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
    };

    template<usize I>
    using arg_type = typename args<I>::type;

};

template<typename T>
concept is_iterable = requires(T t) {
    t.begin();
    t.end();
};

template<typename T>
concept has_at_end = requires(T t) {
    { t.at_end() } -> std::same_as<bool>;
};

template<typename T>
concept has_size = requires(T t) {
    { t.size() } -> std::convertible_to<usize>;
};

template<typename T>
concept has_reserve = requires(T t) {
    t.reserve(std::declval<usize>());
};

template<typename T>
concept has_resize = requires(T t) {
    t.resize(std::declval<usize>());
};

template<typename T>
concept has_emplace_back = requires(T t) {
    t.emplace_back();
};

template<typename T>
concept has_clear = requires(T t) {
    t.clear();
};

template<typename T>
concept has_make_empty = requires(T t) {
    t.make_empty();
};

template<typename T, typename U>
concept has_append = requires(T t, U u) {
    t.append(u);
};


template<typename T> requires(is_iterable<T>)
using element_type_t = std::remove_cvref_t<decltype(*std::declval<T>().begin())>;


}


#endif // Y_UTILS_TRAITS_H

