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

template<typename T>
inline auto hash(const T& t);

// from boost
template<typename T>
inline void hash_combine(T& seed, T value) {
	seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<typename T>
inline auto hash_one(const T& t) {
	return std::hash<T>()(t);
}

template<typename T>
inline auto hash_iterable(const T& collection) {
	using value_type = typename T::value_type;
	decltype(std::hash<value_type>()(*collection.begin())) seed = 0;
	for(const auto& i : collection) {
		hash_combine(seed, hash(i));
	}
	return seed;
}

template<typename T>
inline auto hash(const T& t) {
	if constexpr(is_iterable<std::remove_reference_t<T>>::value) {
		return hash_iterable(t);
	} else {
		return hash_one(t);
	}
}

}

template<typename T, typename... Args>
inline auto hash(const T& t, const Args&... args) {
	auto h = detail::hash(t);
	if constexpr(sizeof...(args)) {
		detail::hash_combine(h, hash(args...));
	}
	return h;
}

struct Hash {
	template<typename... Args>
	auto operator()(const Args&... args) const {
		return hash(args...);
	}
};

}


/*namespace std {
	template<typename T>
	struct hash {
		typedef T argument_type;
		typedef std::size_t result_type;
		template<typename = std::enable_if_t<y::is_iterable<T>::value, T>::type>
		result_type operator()(const argument_type& collection) const {
			result_type seed = 0;
			for(const auto& i : collection) {
				y::detail::hash_combine(seed, y::hash(i));
			}
			return seed;
		}
	};
}*/

#endif // Y_UTILS_HASH_H
