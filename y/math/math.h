/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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
#ifndef Y_MATH_MATH_H
#define Y_MATH_MATH_H

#include <y/utils.h>
#include "Matrix.h"

namespace y {
namespace math {

template<typename T>
struct to_floating_point {
	using same_size = typename std::conditional<(sizeof(T) > sizeof(float)), double, float>::type;
	using type = typename std::conditional<std::is_floating_point<T>::value, T, same_size>::type;
};

template<typename T>
auto to_rad(T deg) {
	return deg * typename to_floating_point<T>::type(0.01745329251994329576923690768489);
}


template<typename T>
auto to_deg(T rad) {
	static_assert(std::is_floating_point<T>::value, "to_deg only takes floating points arguments");
	return rad * T(57.295779513082320876798154814105);
}





// matrix funcs from https://github.com/g-truc/glm/blob/master/glm/gtc/matrix_transform.inl
template<typename T>
auto perspective(T fovy, T aspect, T z_near, T z_far) {
		T const tan_half = tan(fovy / T(2));

		Matrix4<T> m = identity();
		m[0][0] = T(1) / (aspect * tan_half);
		m[1][1] = T(1) / tan_half;
		// m[1][1][1][1] *= -1; //----------------------------------------------------------------
		m[2][3] = -T(1);
		m[2][2] = -(z_far + z_near) / (z_far - z_near);
		m[3][2] = -(T(2) * z_far * z_near) / (z_far - z_near);

		return m;
}

template<typename T>
auto look_at(const Vec<3, T>& eye, const Vec<3, T>& center, const Vec<3, T>& up = Vec<3, T>(0, 0, 1)) {
		Vec<3, T> f((center - eye).normalized());
		Vec<3, T> s(f.cross(up).normalized());
		Vec<3, T> u(s.cross(f));

		Matrix4<T> m = identity();
		m[0][0] = s.x();
		m[1][0] = s.y();
		m[2][0] = s.z();
		m[0][1] = u.x();
		m[1][1] = u.y();
		m[2][1] = u.z();
		m[0][2] =-f.x();
		m[1][2] =-f.y();
		m[2][2] =-f.z();
		m[3][0] =-s.dot(eye);
		m[3][1] =-u.dot(eye);
		m[3][2] = f.dot(eye);

		return m;
}

template<typename T>
auto rotation(T angle, Vec<3, T> axis, const Matrix4<T>& base = identity()) {
		T a = angle;
		T c = cos(a);
		T s = sin(a);

		axis.normalize();
		Vec<3, T> tmp((T(1) - c) * axis);

		Matrix4<T> m;
		m[0][0] = c + tmp[0] * axis[0];
		m[0][1] = tmp[0] * axis[1] + s * axis[2];
		m[0][2] = tmp[0] * axis[2] - s * axis[1];

		m[1][0] = tmp[1] * axis[0] - s * axis[2];
		m[1][1] = c + tmp[1] * axis[1];
		m[1][2] = tmp[1] * axis[2] + s * axis[0];

		m[2][0] = tmp[2] * axis[0] + s * axis[1];
		m[2][1] = tmp[2] * axis[1] - s * axis[0];
		m[2][2] = c + tmp[2] * axis[2];

		Matrix4<T> r;
		r[0] = base[0] * m[0][0] + base[1] * m[0][1] + base[2] * m[0][2];
		r[1] = base[0] * m[1][0] + base[1] * m[1][1] + base[2] * m[1][2];
		r[2] = base[0] * m[2][0] + base[1] * m[2][1] + base[2] * m[2][2];
		r[3] = base[3];

		return r;
}

}
}

#endif // Y_MATH_MATH_H
