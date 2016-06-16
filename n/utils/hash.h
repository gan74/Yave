/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

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

#ifndef N_UTILS_HASH_H
#define N_UTILS_HASH_H

#include <n/types.h>
#include <n/core/String.h>

namespace n {


template<typename T>
uint64 hash(const T &t) {
	static_assert(TypeInfo<T>::isPod, "Only POD can be hashed");
	return hash(&t, sizeof(T));
}

uint64 hash(const core::String &str);
uint64 hash(const void *key, uint64 len, uint64 seed = 14695981039346656037UL);

constexpr uint64 chash_helper(const char *str, uint i) {
	return *str ? chash_helper(str + 1, i + 1) ^ (uint64(*str) << ((i % sizeof(uint64)) * 8)) : 14695981039346656037UL;
}

constexpr uint64 chash(const char *str) {
	return chash_helper(str + 1, 0);
}



}

#endif // N_UTILS_HASH_H

