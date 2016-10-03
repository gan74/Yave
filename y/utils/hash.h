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
#ifndef Y_UTILS_HASH_H
#define Y_UTILS_HASH_H

#include "iterable.h"
#include <cstring>

namespace y {

namespace detail {

// from boost
template<typename T>
inline void hash_combine(T& seed, T value) {
	seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

}

template<typename T, typename Hasher = std::hash<T>>
static auto hash(const T& t, const Hasher& hasher = Hasher()) {
	return hasher(t);
}

template<typename Hasher = std::hash<u32>>
static auto hash(const u32* t, usize size, const Hasher& hasher = Hasher()) {
	decltype(hasher(make_one<u32>())) seed = 0;
	for(usize i = 0; i != size; i++) {
		detail::hash_combine(seed, hash(t[i], hasher));
	}
	return seed;
}
}


namespace std {
	template<typename T>
	struct hash {
		typedef T argument_type;
		typedef std::size_t result_type;
		template<typename Enable = typename std::enable_if<y::is_iterable<T>::value, T>::type>
		result_type operator()(const argument_type& collection) const {
			result_type seed = 0;
			for(const auto& i : collection) {
				y::detail::hash_combine(seed, y::hash(i));
			}
			return seed;
		}
	};
}

#endif // Y_UTILS_HASH_H
