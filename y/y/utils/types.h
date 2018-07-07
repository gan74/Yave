/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#ifndef Y_UTILS_TYPES_H
#define Y_UTILS_TYPES_H

#include <cstdint>
#include <utility>

#include "detect.h"

namespace y {

struct NonCopyable {
	constexpr NonCopyable() {}
	NonCopyable(const NonCopyable &) = delete;
	NonCopyable& operator=(const NonCopyable &) = delete;

	//NonCopyable(NonCopyable&&) {}
};

static_assert(!std::is_move_assignable_v<NonCopyable>);

using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using usize = std::make_unsigned<std::size_t>::type;
using isize = std::make_signed<std::size_t>::type;


template<typename T>
using Owner = T;

template<typename T>
using NotOwner = T;


namespace detail {
enum Enum { _ = u32(-1) };
}

using uenum = std::underlying_type<detail::Enum>::type;



template<bool B>
using bool_type = typename std::integral_constant<bool, B>;



// type traits

/*namespace detail {
template<typename R, typename... Args>
std::true_type is_function_pointer_test_inner(R (*)(Args...));
std::false_type is_function_pointer_test_inner(...);

template<typename T, typename P = decltype(&T::operator+)>
auto is_function_pointer_test(P) -> decltype(is_function_pointer_test_inner(+std::declval<T>()));
template<typename T>
auto is_function_pointer_test(...) -> decltype(is_function_pointer_test_inner(std::declval<T>()));


template<typename T>
static auto has_unary_plus(T*) -> bool_type<!std::is_void_v<decltype(std::declval<T>().operator+())>>;
template<typename T>
static auto has_unary_plus(...) -> std::false_type;
}

template<typename T>
using is_function_pointer = decltype(detail::is_function_pointer_test<T>(nullptr));
template<typename T>
static constexpr bool is_function_pointer_v = is_function_pointer<T>::value;

namespace {
	static const auto lambda = [] { return 0; };
	static_assert(is_function_pointer<decltype(lambda)>::value);
	static_assert(!is_function_pointer<int>::value);
	static_assert(!is_function_pointer<void*>::value);
	static_assert(is_function_pointer<void (*)(int)>::value);
}*/

/*template<typename T>
struct function_t : function_t<decltype(&T::operator())> {};

template<typename R, typename... Args>
struct function_t<R (*)(Args...)> {
	using return_type = R;
	using argument_tuple = std::tuple<Args...>;
};*/

template<typename T>
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
		decltype(detail::has_begin<T>(nullptr))::value &&
		decltype(detail::has_end<T>(nullptr))::value
	>;

namespace detail {
template<typename T>
using has_size_t = decltype(std::declval<T&>().size());
template<typename T>
using has_reserve_t = decltype(std::declval<T&>().reserve(std::declval<usize>()));
}

template<typename T>
void try_reserve(T& t, usize size) {
	if constexpr(is_detected_v<detail::has_reserve_t, T>) {
		t.reserve(size);
	}
}

}


#endif // Y_UTILS_TYPES_H
