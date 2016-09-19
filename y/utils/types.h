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
#ifndef Y_UTILS_TYPES_H
#define Y_UTILS_TYPES_H

#include <cstdint>
#include <utility>

namespace y {

struct NonCopyable
{
	NonCopyable() {}
	NonCopyable(const NonCopyable &) = delete;
	NonCopyable& operator=(const NonCopyable &) = delete;
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


template<typename T>
using Owned = T;

template<typename T>
using NotOwned = T;


namespace detail {
enum Enum { _ = u32(-1) };
}

using uenum = std::underlying_type<detail::Enum>::type;



template<bool B>
using bool_type = typename std::integral_constant<bool, B>;



// short hand for std::declval
template<typename T>
T& make_one();

}


#endif // Y_UTILS_TYPES_H
