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
#ifndef Y_UTILS_DEREF_H
#define Y_UTILS_DEREF_H

#include "types.h"

namespace y {

namespace detail {

// from somewhere on stack overflow (can't find the page anymore)
struct DerefTag {
	template<typename T>
	DerefTag(const T &);
};

// This operator will be used if there is no 'real' operator
DerefTag operator*(const DerefTag& );

// This is need in case of operator * return type is void
//DerefTag operator,(DerefTag, int);

std::false_type (&deref_helper(DerefTag));

template<typename T>
std::true_type deref_helper(const T&);

template<typename T, typename Enable = void>
struct DerefTrait {
	using type = decltype(deref_helper((**make_one<T*>())));
	//using type = std::false_type;
};

// specialization for void pointers
template<typename T>
struct DerefTrait<T,
	typename std::enable_if<std::is_void<typename std::remove_cv<typename std::remove_pointer<T>::type>::type>::value>::type> {
	using type = std::false_type;
};

// specialization for arithmetic types
template<typename T>
struct DerefTrait<T,
	typename std::enable_if<std::is_arithmetic<typename std::remove_cv<T>::type>::value>::type> {
	using type = std::false_type;
};

}

template<typename T>
using is_dereferenceable = typename detail::DerefTrait<T>::type;

}


#endif // Y_UTILS_DEREF_H
