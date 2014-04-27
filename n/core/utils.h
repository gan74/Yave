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

#include <iostream>
#include <n/Types.h>

#define N_PI 3.1415926535897932384626433832795028841971693993751058

#define nUnused(var) (void)var

#ifdef N_DEBUG
#define nError(msg) std::cerr<<((msg))<<" in "<<__FILE__<<" at line : "<<__LINE__<<std::endl; exit(1)
#else
#define nError(msg) std::cerr<<((msg))<<std::endl; exit(1)
#endif

#ifndef __GNUC__
#define N_NO_FORCE_INLINE
#endif

#ifdef N_NO_FORCE_INLINE
#define N_FORCE_INLINE /*nothing*/
#else
#define N_FORCE_INLINE __attribute__((always_inline))
#endif

namespace n
{
namespace core
{

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

template <typename T>
T sign(T x) {
	return (T(0) < x) - (x < T(0));
}

uint hash(const void *key, uint len, uint seed = 0x1000193);
uint log2ui(uint n);


} //core
} //n

#endif // N_CORE_UTILS_H
