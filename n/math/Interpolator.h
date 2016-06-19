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

#ifndef N_MATH_INTERPOLATOR_H
#define N_MATH_INTERPOLATOR_H

#include <n/core/Array.h>
#include <n/core/Pair.h>
#include "Vec.h"
#include "PrecomputedRange.h"

namespace n {
namespace math {

template<typename T, typename K = float>
class Interpolator
{
	public:
		virtual ~Interpolator() {
		}

		virtual T eval(K key) const = 0;
		virtual PrecomputedRange<T> toPrecomputedRange(uint pCount = 0) const = 0;

		T operator()(K key) const {
			return eval(key);
		}
};

template<typename T, typename K = float>
class LinearInterpolator : public Interpolator<T, K>
{
	using E = core::Pair<K, T> ;
	using Storage = core::Array<E, core::OptimalArrayResizePolicy>;

	public:
		using Element = E;

		LinearInterpolator(const core::Array<E> &controlPts) : Interpolator<T, K>(), pts(controlPts) {
		}

		virtual ~LinearInterpolator() {
		}

		virtual T eval(K key) const final override  {
			if(pts.size() == 1) {
				return pts.first()._2;
			}
			if(key <= pts.first()._1) {
				return pts.first()._2;
			}
			if(key >= pts.last()._1) {
				return pts.last()._2;
			}
			if(pts.size() == 2) {
				return lerp(pts.first(), pts.last(), key);
			}
			uint it = find(key);
			return lerp(pts[it], pts[it + 1], key);
		}

		virtual PrecomputedRange<T> toPrecomputedRange(uint pCount = 0) const override {
			pCount = pCount ? pCount : std::min(pts.size() - 1, 512u);
			K incr = (pts.last()._1 - pts.first()._1) / pCount;
			core::Array<T, core::OptimalArrayResizePolicy> distr(pCount + 1);
			for(uint i = 0; i != pCount + 1; i++) {
				distr.append(eval(i * incr));
			}
			return PrecomputedRange<T>(distr);
		}


	private:
		static T lerp(E a, E b, K key) {
			K w = (a._1 - key) / (a._1 - b._1);
			return b._2 * w + a._2 * (1.0 - w);
		}

		uint find(K key) const {
			uint beg = 0;
			uint end = pts.size();
			uint it = 0;
			while(true) {
				it = (end + beg) / 2;
				if(key == pts[it]._1) {
					break;
				} else if(key < pts[it]._1) {
					end = it;
				} else {
					if(key <= pts[it + 1]._1) {
						break;
					}
					beg = it;
				}
			}
			return it;
		}

		Storage pts;
};

}
}


#endif // N_MATH_INTERPOLATOR_H

