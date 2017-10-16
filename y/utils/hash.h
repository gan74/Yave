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
#ifndef Y_UTILS_HASH_H
#define Y_UTILS_HASH_H

#include "iterable.h"

namespace y {

namespace detail {

// from boost
template<typename T>
inline void hash_combine(T& seed, T value) {
	seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

}

template<typename T, typename... Args>
inline auto hash(const T& t, const Args&... args) {
	auto h = std::hash<T>()(t);
	if constexpr(sizeof...(args)) {
		detail::hash_combine(h, hash(args...));
	}
	return h;
}

template<typename B, typename E>
inline auto hash_range(B begin, const E& end) {
	decltype(hash(*begin)) h = 0;
	for(; begin != end; ++begin) {
		detail::hash_combine(h, hash(*begin));
	}
	return h;
}

template<typename C>
inline auto hash_range(const C& c) {
	return hash_range(std::begin(c), std::end(c));
}

}


#endif // Y_UTILS_HASH_H
