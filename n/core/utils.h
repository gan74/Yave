/*******************************
Copyright (C) 2009-2010 grégoire ANGERAND

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

#ifndef N_CORE_UTILS_H
#define N_CORE_UTILS_H

#include <n/Types.h>
#include <tgmath.h>

namespace n {
namespace core {

extern const void *null;

uint random(uint max, uint min = 0);
float random();

float toDeg(float a);
float toRad(float a);

union nColor
{
	nColor() : id(0) {
	}

	nColor(byte R, byte G, byte B, byte A = 255) : rgba({R, G, B, A}) {
	}

	struct
	{
		byte r;
		byte g;
		byte b;
		byte a;
	} rgba;
	uint id;
	float real;
	byte bytes[4];
};

template<typename T>
const T &clamp(const T &val, const T &mi, const T &ma) {
	return val < mi ? mi : (val < ma) ? val : ma;
}

template<typename T>
bool isInRange(const T &v, const T &min, const T &max) {
	return min <= v  && v < max;
}

template<typename T>
T sign(T x) {
	return (T(0) < x) - (x < T(0));
}

uint hash(const void *key, uint len, uint seed = 0x1000193);
uint log2ui(uint n);


} //core
} //n

#endif // N_CORE_UTILS_H
