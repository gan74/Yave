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

template<typename T, typename Hasher>
inline auto hash(const T& t, const Hasher& hasher, std::false_type) {
	return hasher(t);
}

template<typename T, typename Hasher>
inline auto hash(const T& collection, const Hasher& hasher, std::true_type) {
	decltype(hasher(*collection.begin())) seed = 0;
	for(const auto& i : collection) {
		detail::hash_combine(seed, hash(i, hasher, std::false_type()));
	}
	return seed;
}

}

template<typename T, typename Hasher = typename std::hash<typename std::conditional<is_iterable<T>::value, typename std::remove_cv<typename std::remove_reference<decltype(*make_one<T>().begin())>::type>::type, T>::type>>
inline auto hash(const T& t, const Hasher& hasher = Hasher()) {
	return detail::hash(t, hasher, is_iterable<T>());
}

template<typename T>
struct hash_t {
	auto operator()(const T& arg) const {
		return hash(arg);
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
