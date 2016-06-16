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

#ifndef N_MATH_RAY_H
#define N_MATH_RAY_H

#include "Volume.h"

namespace n {
namespace math {

template<typename T = float>
class Ray final : public Volume<T>
{
	public:
		Ray(const Vec<3, T> &beg, const Vec<3, T> &dir) : b(beg), d(dir.normalized()) {
		}

		virtual bool isInside(const Vec<3, T> &v, T r) const override {
			Vec<3, T> p(v - b);
			T dot = d.dot(p);
			return p.length2() - dot * dot < (r * r);
		}

		T distance(const Vec<3, T> &v) const {
			Vec<3, T> p(v - b);
			T dot = d.dot(p);
			return std::sqrt(p.length2() - dot * dot);
		}

		const Vec<3, T> &getDirection() const {
			return d;
		}

		const Vec<3, T> &getStart() const {
			return b;
		}

	private:
		Vec<3, T> b;
		Vec<3, T> d;
};

}
}

#endif // N_MATH_RAY_H

