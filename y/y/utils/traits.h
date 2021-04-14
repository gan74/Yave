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
#ifndef Y_UTILS_TRAITS_H
#define Y_UTILS_TRAITS_H

#include "types.h"
#include "detect.h"

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



template<bool B>
using bool_type = typename std::integral_constant<bool, B>;

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

    static constexpr usize n_args = sizeof...(Args);

    using argument_pack = std::tuple<std::remove_const_t<std::remove_reference_t<Args>>...>;

    template<usize I>
    struct args {
        using type = typename std::tuple_element<I, std::tuple<Args...>>::type;
    };

    template<usize I>
    using arg_type = typename args<I>::type;

};


namespace detail {
template<typename T>
static auto has_begin(T*) -> bool_type<!std::is_void_v<decltype(std::declval<T>().begin())>>;
template<typename T>
static auto has_begin(...) -> std::false_type;

template<typename T>
static auto has_end(T*) -> bool_type<!std::is_void_v<decltype(std::declval<T>().end())>>;
template<typename T>
static auto has_end(...) -> std::false_type;
}

template<typename T>
using is_iterable = bool_type<
        decltype(detail::has_begin<std::remove_reference_t<T>>(nullptr))::value &&
        decltype(detail::has_end<std::remove_reference_t<T>>(nullptr))::value
    >;

template<typename T>
static constexpr bool is_iterable_v = is_iterable<T>::value;

template<typename T>
using remove_cvref = std::remove_cv<std::remove_reference_t<T>>;

template<typename T>
using remove_cvref_t = typename remove_cvref<T>::type;



namespace detail {
template<typename T>
using has_at_end_t = decltype(std::declval<T&>().at_end());
template<typename T>
using has_size_t = decltype(std::declval<T&>().size());
template<typename T>
using has_reserve_t = decltype(std::declval<T&>().reserve(std::declval<usize>()));
template<typename T>
using has_resize_t = decltype(std::declval<T&>().resize(std::declval<usize>()));
template<typename T>
using has_emplace_back_t = decltype(std::declval<T&>().emplace_back());
template<typename T>
using has_clear_t = decltype(std::declval<T&>().clear());
template<typename T>
using has_make_empty_t = decltype(std::declval<T&>().make_empty());
}

template<typename T>
static constexpr bool has_at_end_v = is_detected_v<detail::has_at_end_t, T>;
template<typename T>
static constexpr bool has_size_v = is_detected_v<detail::has_size_t, T>;
template<typename T>
static constexpr bool has_reserve_v = is_detected_v<detail::has_reserve_t, T>;
template<typename T>
static constexpr bool has_resize_v = is_detected_v<detail::has_resize_t, T>;
template<typename T>
static constexpr bool has_emplace_back_v = is_detected_v<detail::has_emplace_back_t, T>;
template<typename T>
static constexpr bool has_clear_v = is_detected_v<detail::has_clear_t, T>;
template<typename T>
static constexpr bool has_make_empty_v = is_detected_v<detail::has_make_empty_t, T>;
}


#endif // Y_UTILS_TRAITS_H

