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

#include "hash.h"
#include <n/core/String.h>

namespace n {

uint64 hash(const core::String &str) {
	return str.getHash();
}

uint64 hash(const void *key, uint64 len, uint64 seed) { // FVN-1A from : http://isthe.com/chongo/tech/comp/fnv/
	constexpr uint64 prime = 1099511628211;
	uint64 h = seed;
	for(const byte *i = static_cast<const byte *>(key); i != static_cast<const byte *>(key) + len; i++) {
		h ^= uint64(*i);
		h *= prime;
	}
	return h;
}

}
