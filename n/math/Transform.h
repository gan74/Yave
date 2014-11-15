/*******************************
Copyright (C) 2013-2014 gr�goire ANGERAND

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

#ifndef N_MATH_TRANSFORM_H
#define N_MATH_TRANSFORM_H

#include "Quaternion.h"
#include "Matrix.h"

namespace n {
namespace math {

template<typename T = float>
class Transform
{
	public:
		Transform() {
		}

		Transform(const Quaternion<T> &q, const Vec<3, T> &p = Vec<3, T>()) : pos(p), rot(q) {
		}

		Transform(const Vec<3, T> &p) : pos(p) {
		}

		Vec<3, T> toLocal(const Vec<3, T> &v) const {
			return -rot(v - pos);
		}

		Vec<3, T> toGlobal(const Vec<3, T> &v) const {
			return pos + rot(v);
		}

		Vec<3, T> getX() const {
			return rot(Vec<3, T>(1, 0, 0));
		}

		Vec<3, T> getY() const {
			return rot(Vec<3, T>(0, 1, 0));
		}

		Vec<3, T> getZ() const {
			return rot(Vec<3, T>(0, 0, 1));
		}

		Quaternion<T> getRotation() const {
			return rot;
		}

		Matrix<3, 3, T> getBasis() const {
			return Matrix<3, 3, T>(getX(), getY(), getZ()).transposed();
		}

		Matrix<4, 4, T> getMatrix() const {
			return Matrix<4, 4, T>(getX(), 0,
								   getY(), 0,
								   getY(), 0,
								   pos, 1).transposed();
		}

	private:
		Vec<3, T> pos;
		Quaternion<T> rot;
};

}
}


#endif // TRANSFORM_H
