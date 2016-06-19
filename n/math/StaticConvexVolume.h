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

#ifndef N_MATH_STATICCONVEXVOLUME_H
#define N_MATH_STATICCONVEXVOLUME_H

#include "Plane.h"

namespace n {
namespace math {

template<uint N, typename T = float>
class StaticConvexVolume final : public Volume<T>
{
	public:
		using const_iterator = const Plane<T> *;

		StaticConvexVolume(Plane<T> pls[N]) {
			for(uint i = 0; i != N; i++) {
				planes[i] = pls[i];
			}
		}

		const_iterator begin() const {
			return &planes;
		}

		const_iterator end() const {
			return &planes + N;
		}

		virtual bool isInside(const Vec<3, T> &v, T r) const override {
			for(const Plane<T> &p : *this) {
				if(!p.isInside(v, r)) {
					return false;
				}
			}
			return N;
		}

	private:
		Plane<T> planes[N];
};

}
}


#endif // N_MATH_STATICCONVEXVOLUME_H

