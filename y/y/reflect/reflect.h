/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include <y/core/String.h>

#include <y/utils/recmacros.h>
#include <y/utils/detect.h>

#include <tuple>
#include <type_traits>
#include <string_view>

namespace y {
namespace reflect {

#define y_reflect_unfold_visit(N)													\
	do {																			\
		y::reflect::reflect(_y_reflect_reflector, N, std::string_view(#N));			\
	} while(false);


#define y_qualified_reflect(qualifiers, ...)								\
	template<typename R>													\
	void reflect(R&& _y_reflect_reflector) qualifiers {						\
		Y_REC_MACRO(Y_MACRO_MAP(y_reflect_unfold_visit, __VA_ARGS__))		\
	}


#define y_reflect(...)														\
	y_qualified_reflect(const, __VA_ARGS__)									\
	y_qualified_reflect(/* */, __VA_ARGS__)



template<typename R, typename T>
void reflect(R&&, T&&, std::string_view = "");

namespace detail {

template<typename T, typename R>
using has_reflect_t = decltype(std::declval<T>().reflect(std::declval<R>()));
template<typename T, typename R>
using has_post_reflect_t = decltype(std::declval<T>().post_reflect(std::declval<R>()));


template<typename U>
struct Reflector {
	template<typename R, typename T>
	static void recurse(R&& reflector, T&& obj) {
		if constexpr(is_detected_v<has_reflect_t, T, R>) {
			obj.reflect(reflector);
		} else if constexpr(is_iterable_v<T>) {
			usize index = 0;
			for(auto&& item : obj) {
				core::String buffer;
				fmt_into(buffer, "[%]", index++);
				reflect(reflector, item, buffer);
			}
		}
	}
};

template<typename... Args>
struct Reflector<std::tuple<Args...>> {
	template<usize I, typename R, typename T>
	static void recurse_tuple(R&& reflector, T&& obj) {
		if constexpr(I < sizeof...(Args)) {
			core::String buffer;
			fmt_into(buffer, "std::get<%>", I);
			reflect(reflector, std::get<I>(obj), buffer);
			recurse_tuple<I + 1>(reflector, obj);
		}
	}

	template<typename R, typename T>
	static void recurse(R&& reflector, T&& obj) {
		recurse_tuple<0>(reflector, obj);
	}
};

template<typename A, typename B>
struct Reflector<std::pair<A, B>> {
	template<typename R, typename T>
	static void recurse(R&& reflector, T&& obj) {
		reflect(reflector, obj.first, "first");
		reflect(reflector, obj.second, "second");
	}
};

}


template<typename R, typename T>
void reflect(R&& reflector, T&& obj, std::string_view name) {
	using refl_t = decltype(reflector);
	using obj_t = decltype(obj);
	static_assert(std::is_invocable_v<refl_t, std::string_view, obj_t>);

	if constexpr(std::is_invocable_r_v<bool, refl_t, obj_t>) {
		if(!reflector(name, obj)) {
			return;
		}
	} else {
		reflector(name, obj);
	}

	detail::Reflector<std::remove_reference_t<T>>::recurse(reflector, obj);

	if constexpr(is_detected_v<detail::has_post_reflect_t, refl_t, obj_t>) {
		reflector.post_reflect(obj);
	}
}

}
}



#endif // Y_REFLECT_REFLECT_H
