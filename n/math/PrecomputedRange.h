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

#ifndef N_MATH_PRECOMPUTEDRANGE_H
#define N_MATH_PRECOMPUTEDRANGE_H

#include <n/core/Array.h>

namespace n {
namespace math {

template<typename T>
class PrecomputedRange // assumed continuous and key in [0, 1]
{
	public:
		PrecomputedRange(const core::Array<T> &values) : pts(values) {
		}

		PrecomputedRange(const T &u) : pts({u}) {
		}

		template<typename K>
		T eval(K key) const {
			if(pts.size() == 1) {
				return pts.first();
			}
			if(pts.size() == 2) {
				return pts.first() * (1.0 - key) + pts.last() * key;
			}
			key *= (pts.size() - 1);
			uint fl = key;
			K w = key - fl;
			return pts[fl] * (1.0 - w) + pts[fl + 1] * w;
		}

		template<typename K>
		T operator()(K key) const {
			return eval(key);
		}

		T operator[](uint i) const {
			return pts[i % pts.size()];
		}

		uint size() const {
			return pts.size();
		}

		typename core::Array<T>::const_iterator begin() const {
			return pts.begin();
		}

		typename core::Array<T>::const_iterator end() const {
			return pts.end();
		}

	private:
		core::Array<T, core::OptimalArrayResizePolicy> pts;
};

}
}

#endif // N_MATH_PRECOMPUTEDRANGE_H

