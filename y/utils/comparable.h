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
#ifndef Y_UTILS_COMPARABLE_H
#define Y_UTILS_COMPARABLE_H

#include "types.h"

namespace y {

namespace detail {

template<typename T, typename To>
struct is_comparable_SFINAE {

	template<typename V>
	static auto test(V*) -> std::is_same<decltype(make_one<V>().operator==(make_one<To>())), bool>;

	template<typename V>
	static std::false_type test(...);

	static constexpr bool value = decltype(test<T>(0))::value;
};

}

template<typename T, typename To>
using is_comparable = bool_type<detail::is_comparable_SFINAE<T, To>::value>;

}

#endif // Y_UTILS_COMPARABLE_H
