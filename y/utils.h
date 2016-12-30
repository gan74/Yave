/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#ifndef Y_UTILS_H
#define Y_UTILS_H

#include "defines.h"
#include <new>
#include <typeinfo>
#include <tuple>
#include <array>

#include <y/utils/types.h>

#include <y/utils/deref.h>
#include <y/utils/iterable.h>
#include <y/utils/comparable.h>
#include <y/utils/startup.h>
#include <y/utils/Chrono.h>
#include <y/utils/hash.h>
#include <y/utils/log.h>
#include <y/utils/os.h>

namespace y {





template<typename T>
struct dereference {
	using type = decltype(*make_one<T>());
};




struct Nothing {
	template<typename... Args>
	Nothing(Args...) {
	}

	template<typename... Args>
	Nothing operator()(Args...) const {
		return *this;
	}

	template<typename... Args>
	Nothing operator()(Args...) {
		return *this;
	}

	template<typename T>
	operator T() const {
		return fatal("y::detail::Nothing used");
	}
};




constexpr usize log2ui(usize n) {
	return (n >> 1) ? log2ui(n >> 1) + 1 : 0;
}




template<typename... Args>
constexpr void unused(Args...) {}



namespace detail {

struct StaticCounter {
	static usize value;
};

template<typename T>
struct TypeUid : private detail::StaticCounter {
	static usize value() {
		static usize value = StaticCounter::value++;
		return value;
	}
};

}


template<typename T>
usize type_uid() {
	return detail::TypeUid<T>::value();
}

template<typename T>
usize type_uid(const T&) {
	return detail::TypeUid<T>::value();
}




constexpr bool is_64_bits() {
	return sizeof(void*) == 8;
}

constexpr bool is_32_bits() {
	return sizeof(void*) == 4;
}




namespace detail {
	static constexpr u32 endian = 0x01020304; // http://stackoverflow.com/questions/1583791/constexpr-and-endianness
	static constexpr u32 endianness = static_cast<const u8 &>(endian);
}

constexpr bool is_little_endian() {
	return detail::endianness == 0x04;
}

constexpr bool is_big_endian() {
	return detail::endianness == 0x01;
}

static_assert(is_little_endian() || is_big_endian(), "Unable to determine endianness");




namespace detail {

template<typename T>
class ScopeExit {
	public:
		ScopeExit(T t) : ex(t) {
		}

		~ScopeExit() {
			ex();
		}

	private:
		T ex;
};

}

template<typename T>
auto scope_exit(T t) {
	return detail::ScopeExit<T>(t);
}

template<typename... Types>
struct Coerce {
};

template<typename T>
struct Coerce<T> {
	using type = T;
};

template<typename T, typename U>
struct Coerce<T, U> {
	using type =  decltype(make_one<T>() + make_one<U>());
};

template<typename T, typename U, typename... Types>
struct Coerce<T, U, Types...> {
	using right = typename Coerce<U, Types...>::type;
	using type = typename Coerce<T, right>::type;
};

static_assert(std::is_same<Coerce<int, float, double>::type, double>::value, "Coerce error");
static_assert(std::is_same<Coerce<int, float, int>::type, float>::value, "Coerce error");
static_assert(std::is_same<Coerce<int, int>::type, int>::value, "Coerce error");




template<typename T>
struct is_sane {
	static constexpr bool is_polymorphic = std::is_polymorphic<T>::value;
	static constexpr bool is_copyable = std::is_copy_constructible<T>::value;

	static constexpr bool value = (is_polymorphic && !is_copyable) || !is_polymorphic;
	using type = bool_type<value>;
};

namespace detail {
struct TryFailed {
	template<typename T>
	operator T() const {
		return T();
	}

	operator bool() const {
		return false;
	}
};
}

}

#define Y_ASSERT_SANE(type) static_assert(y::is_sane<type>::value, "\"" #type "\" is ill-formed")

#define Y_TRY(expr)	do { if(!(expr)) { return y::detail::TryFailed(); } } while(false)

#endif
