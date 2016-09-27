/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/
#ifndef Y_UTILS_H
#define Y_UTILS_H

#include "defines.h"
#include <new>
#include <typeinfo>
#include <chrono>
#include <tuple>
#include <array>

#include <y/utils/types.h>

#include <y/utils/deref.h>
#include <y/utils/comparable.h>
#include <y/utils/startup.h>
#include <y/utils/Chrono.h>
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


template<typename T, typename Hasher = std::hash<T>>
static auto hash(const T& t, const Hasher& hasher = Hasher()) {
	return hasher(t);
}

}

#define Y_ASSERT_SANE(type) static_assert(y::is_sane<type>::value, "\"" #type "\" is ill-formed")

#define Y_TRY(expr)	do { if(!(expr)) { return y::detail::TryFailed(); } } while(false)

#endif
