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
#ifndef Y_UTILS_ITERABLE_H
#define Y_UTILS_ITERABLE_H

#include "types.h"

namespace y {
namespace detail {

template<typename T>
static auto has_begin(T*) -> bool_type<!std::is_void<decltype(make_one<T>().begin())>::value>;
template<typename T>
static auto has_begin(...) -> std::false_type;

template<typename T>
static auto has_end(T*) -> bool_type<!std::is_void<decltype(make_one<T>().end())>::value>;
template<typename T>
static auto has_end(...) -> std::false_type;

}

template<typename T>
using is_iterable = bool_type<
		decltype(detail::has_begin<T>(nullptr))::value &&
		decltype(detail::has_end<T>(nullptr))::value
	>;


}

#endif // Y_UTILS_ITERABLE_H
