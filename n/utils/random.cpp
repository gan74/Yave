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

#include "random.h"
#include <n/utils.h>
#include <ctime>

namespace n {

uint randHelper() {
	uint bits = log2ui(RAND_MAX);
	uint num = 0;
	for(uint i = 0; i < sizeof(uint) * 8; i += bits) {
		num = (num << bits) | rand();
	}
	return num;
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
	return (randHelper() % (max - min)) + min;
}

float random() {
	return float(randHelper() & 0x7FFFFF) * 1.0f / (0x7FFFFF + 1.0f);
}

}
