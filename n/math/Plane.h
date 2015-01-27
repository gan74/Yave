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

#ifndef N_MATH_PLANE
#define N_MATH_PLANE

#include "Volume.h"
#include "Edge.h"

namespace n {
namespace math {

template<typename T>
class Plane final : public Volume<T>
{
	public:
		Plane(const Vec<4, T> &e) : n(e.sub(3).normalized()), d(e.w()) {
		}

		Plane(const Vec<3, T> &e, T f) : n(e.normalized()), d(f) {
		}

		Plane(const Vec<3, T> &norm, const Vec<3, T> &p) : n(norm.normalized()), d(p.dot(n)) {
		}

		Plane(const Edge<T> &a, const Edge<T> &b) : Plane(a.getVec() ^ b.getVec(), a.getBegin()) {
		}

		virtual typename Volume<T>::IntersectionState isInside(const Vec<3, T> &v, T r) const override {
			T d = getSignedDistance(v);
			if(d > r) {
				return Volume<T>::Outside;
			}
			if(d < -r) {
				return Volume<T>::Inside;
			}
			return Volume<T>::Intersect;
		}

		T getDistance(const Vec<3, T> &v) const {
			T dist = getSignedDistance(v);
			return dist < 0 ? -d : d;
		}

		T getSignedDistance(const Vec<3, T> &v) const {
			return n.dot(v - (n * d));
		}

		const Vec<3, T> &getNormal() const {
			return n;
		}

		T getW() {
			return d;
		}

	private:
		Vec<3, T> n;
		T d;
};

}
}

#endif // N_MATH_PLANE

