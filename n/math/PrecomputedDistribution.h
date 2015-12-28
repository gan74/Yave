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

#ifndef N_MATH_PRECOMPUTEDDISTRIBUTION
#define N_MATH_PRECOMPUTEDDISTRIBUTION

#include <n/core/Array.h>
#include <vector>

namespace n {
namespace math {

template<typename T = float>
class PrecomputedDistribution // assumed continuous and key in [0, 1]
{
	public:
		PrecomputedDistribution(const core::Array<T> &values) : pts(values) {
		}

		T eval(T key) const {
			if(pts.size() == 1) {
				return pts.first();
			}
			key *= (pts.size() - 1);
			uint fl = key;
			T w = key - fl;
			return pts[fl] * (1.0 - w) + pts[fl + 1] * w;
		}

		T operator()(T key) const {
			return eval(key);
		}

		uint size() const {
			return pts.size();
		}

	private:
		core::Array<T, core::OptimalArrayResizePolicy> pts;
};

}
}

#endif // N_MATH_PRECOMPUTEDDISTRIBUTION

