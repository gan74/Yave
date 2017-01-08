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

template<typename T, typename = void>
struct DerefTrait {
	using type = decltype(deref_helper((**make_one<T*>())));
	//using type = std::false_type;
};

// specialization for void pointers
template<typename T>
struct DerefTrait<T,
	std::enable_if_t<std::is_void<typename std::remove_cv<typename std::remove_pointer<T>::type>::type>::value>> {
	using type = std::false_type;
};

// specialization for arithmetic types
template<typename T>
struct DerefTrait<T,
	std::enable_if_t<std::is_arithmetic<typename std::remove_cv<T>::type>::value>> {
	using type = std::false_type;
};

}

template<typename T>
using is_dereferenceable = typename detail::DerefTrait<T>::type;

}


#endif // Y_UTILS_DEREF_H
