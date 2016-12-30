/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

This code is licensed under the MIT License (MIT).

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef Y_MATH_MATH_H
#define Y_MATH_MATH_H

#include <y/utils.h>
#include <limits>
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


template<typename T>
static constexpr T epsilon = std::numeric_limits<T>::epsilon();

template<typename T = double>
static constexpr T pi = T(3.1415926535897932384626433832795);


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
		m[0][2] = -f.x();
		m[1][2] = -f.y();
		m[2][2] = -f.z();
		m[3][0] = -s.dot(eye);
		m[3][1] = -u.dot(eye);
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
