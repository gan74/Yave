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

#ifndef N_MATH_SPHERE_H
#define N_MATH_SPHERE_H

#include "Volume.h"

namespace n {
namespace math {

template<typename T>
class Sphere final : public Volume<T>
{
	public:
		Sphere(const Vec<4, T> &e) : p(e), r(e.w()) {
		}

		Sphere(const Vec<3, T> &pos, T rad) : p(pos), r(rad) {
		}

		Sphere(T rad) : r(rad) {
		}

		virtual bool isInside(const Vec<3, T> &v, T rad) const override {
			return (p - v).length2() < ((r + rad) * (r + rad));
		}

		const Vec<3, T> &getPosition() const {
			return p;
		}

		T getRadius() {
			return r;
		}

	private:
		Vec<3, T> p;
		T r;
};

}
}

#endif // N_MATH_SPHERE_H

