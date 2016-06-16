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

#ifndef N_UTILS_MATH_H
#define N_UTILS_MATH_H

#include <numeric>

namespace n {
namespace math {

namespace details {
	template<typename T, bool B>
	struct ToFloatingPoint
	{
		typedef double type;
	};

	template<typename T>
	struct ToFloatingPoint<T, true>
	{
		typedef T type;
	};

	template<typename T, typename U, bool A, bool B>
	struct NormalizedConversion // true true
	{
		T operator()(U u) {
			return u;
		}
	};

	template<typename T, typename U>
	struct NormalizedConversion<T, U, false, false>
	{
		T operator()(U u) {
			if(sizeof(U) > sizeof(T)) {
				return u >> ((sizeof(U) - sizeof(T)) * 8);
			}
			return u << ((sizeof(T) - sizeof(U)) * 8);
		}
	};

	template<typename T, typename U>
	struct NormalizedConversion<T, U, true, false>
	{
		T operator()(U u) {
			return T(u) / T(U(~U(0)));
		}
	};

	template<typename T, typename U>
	struct NormalizedConversion<T, U, false, true>
	{
		T operator()(U u) {
			U max = U(T(~T(0)));
			long double ldm = max;
			long double v = u;
			return T(ldm * v);
		}
	};

}


template<typename T>
struct ToFloatingPoint
{
		typedef typename details::ToFloatingPoint<T, std::is_floating_point<T>::value>::type type;
};

static constexpr double pi = 3.1415926535897932384626433832795028841971693993751058;

template<typename T = double>
static constexpr typename ToFloatingPoint<T>::type epsilon() {
	return std::numeric_limits<typename ToFloatingPoint<T>::type>::epsilon();
}

template<typename T>
typename ToFloatingPoint<T>::type toDeg(T a) {
	typedef typename ToFloatingPoint<T>::type type;
	return type(a) * type(180) / type(pi);
}

template<typename T>
typename ToFloatingPoint<T>::type toRad(T a) {
	typedef typename ToFloatingPoint<T>::type type;
	return type(a) / type(180) * type(pi);
}

template<typename T>
const T &clamp(const T &val, const T &mi, const T &ma) {
	return val < mi ? mi : (val < ma) ? val : ma;
}

template<typename T>
bool isInRange(const T &v, const T &min, const T &max) {
	return (min < v || min == v)  && v < max;
}

template<typename T>
int sign(T x) {
	return int(T(0) < x) - int(x < T(0));
}

template<typename T>
T modPow(T num, T pow, T mod) {
	T exp = 1;
	for(; pow; pow >>= 1) {
		if(pow & 0x01) {
			exp = ((exp % mod) * (num % mod)) % mod;
		}
		num = ((num % mod) * (num % mod)) % mod;
	}
	return exp;
}

template<typename T, typename U>
T normalizedConversion(U u) {
	return details::NormalizedConversion<T, U, std::is_floating_point<T>::value, std::is_floating_point<U>::value>()(u);
}

template<typename T>
T normalizedConversion(T u) {
	return u;
}

}
}

#endif // N_UTILS_MATH_H

