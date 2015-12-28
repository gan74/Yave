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

#ifndef N_MATH_INTERPOLATOR
#define N_MATH_INTERPOLATOR

#include <n/core/Array.h>
#include "Vec.h"
#include "PrecomputedDistribution.h"

namespace n {
namespace math {

template<typename T = float>
class Interpolator
{
	public:
		virtual ~Interpolator() {
		}

		virtual T eval(T key) const = 0;
		virtual PrecomputedDistribution<T> toDistribution(uint pCount = 0) const = 0;

		T operator()(T key) const {
			return eval(key);
		}
};

template<typename T = float>
class LinearInterpolator : public Interpolator<T>
{
	typedef core::Array<Vec<2, T>, core::OptimalArrayResizePolicy> Storage;
	public:
		LinearInterpolator(const core::Array<Vec<2, T>> &controlPts) : Interpolator<T>(), points(controlPts) {
		}

		virtual ~LinearInterpolator() {
		}

		virtual T eval(T key) const final override  {
			if(points.size() == 1) {
				return points.first().y();
			}
			if(key <= points.first().x()) {
				return points.first().y();
			}
			if(key >= points.last().x()) {
				return points.last().y();
			}
			if(points.size() == 2) {
				return lerp(points.first(), points.last(), key);
			}
			typename Storage::const_iterator it = find(key);
			return lerp(*it, *(it + 1), key);
		}

		virtual PrecomputedDistribution<T> toDistribution(uint pCount = 0) const override {
			pCount = pCount ? pCount : 512;
			pCount = pCount > points.size() - 1 ? points.size() - 1 : pCount;
			T incr = (points.last().x() - points.first().x()) / pCount;
			core::Array<T, core::OptimalArrayResizePolicy> distr(pCount + 1);
			for(uint i = 0; i != pCount + 1; i++) {
				distr.append(eval(i * incr));
			}
			return PrecomputedDistribution<T>(distr);
		}


	private:
		static T lerp(Vec<2, T> a, Vec<2, T> b, T key) {
			T w = (a.x() - key) / (a.x() - b.x());
			return b.y() * w + a.y() * (1.0 - w);
		}

		template<typename U>
		static U *middle(U *a, U *b) {
			uint64 ua = uint64(a) / sizeof(U);
			uint64 ub = uint64(b) / sizeof(U);
			uint64 res = (ua + ub) / 2;
			return (U*)(res * sizeof(U));
		}

		typename Storage::const_iterator find(T key) const {
			typename Storage::const_iterator beg = points.begin();
			typename Storage::const_iterator end = points.end();
			typename Storage::const_iterator it;
			while(true) { //*it > key || *(it + 1) < key
				it = middle(beg, end);
				if(key < it->x()) {
					end = it;
				} else {
					if(key <= (it + 1)->x()) {
						break;
					}
					beg = it;
				}
			}
			return it;
		}

		Storage points;
};

}
}


#endif // N_MATH_INTERPOLATOR

