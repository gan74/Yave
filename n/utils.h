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

#ifndef N_UTILS_H
#define N_UTILS_H

#include <n/types.h>
#include <vector>
#include <cfloat>

namespace n {
	static constexpr void *null = 0;

namespace math {
	namespace internal {
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
	}


	template<typename T>
	struct ToFloatingPoint
	{
			typedef typename internal::ToFloatingPoint<T, std::is_floating_point<T>::value>::type type;
	};

	template<typename T = double>
	static constexpr typename ToFloatingPoint<T>::type pi() {
		return typename ToFloatingPoint<T>::type(3.1415926535897932384626433832795028841971693993751058);
	}

	template<typename T = double>
	static constexpr typename ToFloatingPoint<T>::type epsilon() {
		return ToFloatingPoint<T>::type(FLT_EPSILON * 2.0);
	}

	template<typename T>
	typename ToFloatingPoint<T>::type toDeg(T a) {
		typedef typename ToFloatingPoint<T>::type type;
		return a * type(180) / pi<type>();
	}

	template<typename T>
	typename ToFloatingPoint<T>::type toRad(T a) {
		typedef typename ToFloatingPoint<T>::type type;
		return a / type(180) * pi<type>();
	}

	uint random(uint max, uint min = 0);
	float random();

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

	template<typename T>
	T isqrt(T s) {
		T x	= s / 2;
		T lx = x;
		do {
			lx = x;
			x = (x + (s / x)) / 2;
		} while(std::max(x, lx) - std::min(x, lx) > 1);
		return x;
	}
} //math

namespace core {
	class NonCopyable
	{
		public:
			NonCopyable() {}
			NonCopyable(const NonCopyable &) = delete;
			NonCopyable &operator=(const NonCopyable &) = delete;
	};

	uint hash(const void *key, uint len, uint seed = 0x1000193);
	uint log2ui(uint n);

	template<typename I>
	void radixSort(I beg, I end) {
		typedef typename TypeContent<I>::type ElemType;
		static_assert(std::is_integral<ElemType>::value, "Element type needs to be integral");
		std::vector<ElemType> buckets[16];
		for(uint i = 0; i != 2 * sizeof(ElemType); i++) {
			for(I e = beg; e != end; ++e) {
				byte *b = (byte *)&(*e);
				byte bucket = b[i >> 1];
				if(i & 0x01) {
					bucket >>= 4;
				}
				buckets[bucket & 0x0F].push_back(*e);
			}
			I first = beg;
			for(uint j = 0; j != 16; j++) {
				for(const ElemType &e : buckets[j]) {
					*first = e;
					++first;
				}
				buckets[j].clear();
			}
		}
	}
} //core
} //n

#endif // N_UTILS_H
