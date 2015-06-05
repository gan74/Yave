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


#include "utils.h"
#include <n/concurrent/Atomic.h>
#include <n/defines.h>
#include <ctime>
#include <iostream>

namespace n {

Nothing fatal(const char *msg, const char *file, int line) {
	std::cerr<<msg;
	if(file) {
		std::cerr<<" in file "<<file;
		if(line) {
			std::cerr<<" at line "<<line;
		}
	}
	std::cerr<<std::endl;
	exit(1);
	return Nothing();
}

namespace internal {
	uint randHelper() {
		uint bits = core::log2ui(RAND_MAX);
		uint num = 0;
		for(uint i = 0; i < sizeof(uint) * 8; i += bits) {
			num = (num << bits) | rand();
		}
		return num;
	}
}

uint random(uint max, uint min) {
	static bool seed = false;
	if(!seed) {
		#ifdef N_DEBUG
		srand(0);
		#else
		time_t now = time(0);
		srand(hash(&now, sizeof(now)));
		#endif
		seed = true;
	}
	if(max < min) {
		std::swap(min, max);
	}
	return (internal::randHelper() % (max - min)) + min;
}

float random() {
	return (float)(internal::randHelper() & 0x7FFFFF) * 1.0f / (0x7FFFFF + 1.0f);
}

namespace core {
	uint uniqueId() {
		static concurrent::auint counter;
		return ++counter;
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

	void *safeRealloc(void *c, uint size)  {
		void *cc = realloc(c, size);
		if(cc) {
			return cc;
		}
		free(c);
		return 0;
	}
} //core
} //n
