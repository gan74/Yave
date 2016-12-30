/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

This code is licensed under the MIT License (MIT).

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
