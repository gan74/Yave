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

#ifndef N_MATH_STATICCONVEXVOLUME
#define N_MATH_STATICCONVEXVOLUME

#include "Plane.h"

namespace n {
namespace math {

template<uint N, typename T = float>
class StaticConvexVolume final : public Volume<T>
{
	public:
		typedef const Plane<T> * const_iterator;

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

		virtual typename Volume<T>::IntersectionState isInside(const Vec<3, T> &v, T r) const override {
			typename Volume<T>::IntersectionState i = Volume<T>::Inside;
			for(const Plane<T> &p : *this) {
				typename Volume<T>::IntersectionState ir = p.isInside(v, r);
				if(ir == Volume<T>::Outside) {
					return Volume<T>::Outside;
				}
				i |= ir;
			}
			return i & Volume<T>::Intersect ? Volume<T>::Intersect : Volume<T>::Inside;
		}

	private:
		Plane<T> planes[N];
};

}
}


#endif // N_MATH_STATICCONVEXVOLUME

