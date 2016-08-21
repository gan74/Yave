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
#include <cstdint>
#include <utility>
#include <y/utils/deref.h>

namespace y {

struct NonCopyable
{
	NonCopyable() {}
	NonCopyable(const NonCopyable &) = delete;
	NonCopyable &operator=(const NonCopyable &) = delete;
};




using u8 = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;

using usize = std::make_unsigned<size_t>::type;
using isize = std::make_signed<size_t>::type;





template<bool B>
using bool_type = typename std::integral_constant<bool, B>;




template<typename T>
struct dereference {
	using type = decltype(*make_one<T>());
};




struct Nothing {
	template<typename... Args>
	Nothing operator()(Args...) const {
		return *this;
	}

	template<typename T>
	operator T() const {
		return fatal("y::detail::Anything used");
	}
};




constexpr usize log2ui(usize n) {
	return (n >> 1) ? log2ui(n >> 1) + 1 : 0;
}




template<typename... Args>
void unused(Args...) {}



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


}

#endif
