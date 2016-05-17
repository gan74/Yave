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
	return hash(str.data(), str.size());
}

uint64 hash(const void *key, uint64 len, uint64 seed) { // Murmurhash 2.0 from https://sites.google.com/site/murmurhash/
	static_assert(isLittleEndian(), "n::hash doesn't work on non little endian systems");
	const uint64 m = 0xc6a4a7935bd1e995;
	const int r = 47;
	uint64 h = seed ^ (len * m);
	const uint64 * data = (const uint64 *)key;
	const uint64 * end = data + (len / 8);
	while(data != end) {
		uint64 k = *data++;
		k *= m;
		k ^= k >> r;
		k *= m;
		h ^= k;
		h *= m;
	}
	const byte *data2 = (const byte *)data;
	switch(len & 7) {
		case 7: h ^= uint64(data2[6]) << 48;
		case 6: h ^= uint64(data2[5]) << 40;
		case 5: h ^= uint64(data2[4]) << 32;
		case 4: h ^= uint64(data2[3]) << 24;
		case 3: h ^= uint64(data2[2]) << 16;
		case 2: h ^= uint64(data2[1]) << 8;
		case 1: h ^= uint64(data2[0]);
				h *= m;
	};
	h ^= h >> r;
	h *= m;
	h ^= h >> r;
	return h;
}



}
