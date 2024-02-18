/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
    using type = typename std::conditional<std::is_floating_point_v<T>, T, same_size>::type;
};

template<typename T>
inline constexpr auto to_rad(T deg) {
    return deg * typename to_floating_point<T>::type(0.01745329251994329576923690768489);
}


template<typename T>
inline constexpr auto to_deg(T rad) {
    static_assert(std::is_floating_point_v<T>, "to_deg only takes floating points arguments");
    return rad * T(57.295779513082320876798154814105);
}


template<typename T>
static constexpr T epsilon = std::numeric_limits<T>::epsilon();

template<typename T>
static constexpr T pi = T(3.1415926535897932384626433832795);


// http://dev.theomader.com/depth-precision/
// matrix funcs from https://github.com/g-truc/glm/blob/master/glm/gtc/matrix_transform.inl
template<typename T>
auto perspective(T fovy, T aspect, T z_near, T z_far) {
    const T f = T(1) / tan(fovy / T(2));

    // reversed Z
    const auto span = z_near - z_far;
    const Matrix4<T> m(f / aspect, 0, 0, 0,
                 0, f, 0, 0,
                 0, 0, -T(1) - (z_far / span), -(z_far * z_near) / span,
                 0, 0, -T(1), 0);

    return m;
}

// infinite version (no z far)
template<typename T>
auto perspective(T fovy, T aspect, T z_near) {
    const T f = T(1) / std::tan(fovy / T(2));

    // reversed Z
    const Matrix4<T> m(f / aspect, 0, 0, 0,
                 0, f, 0, 0,
                 0, 0, 0, z_near,
                 0, 0, -T(1), 0);
    return m;
}

// https://github.com/g-truc/glm/blob/1498e094b95d1d89164c6442c632d775a2a1bab5/glm/ext/matrix_clip_space.inl
template<typename T>
auto ortho(T left, T right, T bottom, T top, T z_near, T z_far) {
    math::Matrix4<T> m(
        T(2) / (right - left), 0, 0, -(right + left) / (right - left),
        0, T(2) / (top - bottom), 0, -(top + bottom) / (top - bottom),
        0, 0, -T(1) / (z_far - z_near), -z_near / (z_far - z_near),
        0, 0, 0, T(1)
    );
    return m;
}

template<typename T>
auto look_at(const Vec<3, T>& eye, const Vec<3, T>& center, const Vec<3, T>& up) {
    const Vec<3, T> z((eye - center).normalized());
    const Vec<3, T> y(up.cross(z).normalized());
    const Vec<3, T> x(y.cross(z).normalized());

    return Matrix4<T>(y, -y.dot(eye),
                      x, -x.dot(eye),
                      z, -z.dot(eye),
                      0, 0, 0, 1);

}

template<typename T>
auto rotation(Vec<3, T> axis, T angle, const Matrix4<T>& base = identity()) {
    const T a = angle;
    const T c = std::cos(a);
    const T s = std::sin(a);

    axis.normalize();
    const Vec<3, T> tmp((T(1) - c) * axis);

    const Matrix4<T> m(c + tmp[0] * axis[0], tmp[0] * axis[1] + s * axis[2], tmp[0] * axis[2] - s * axis[1], 0,
                 tmp[1] * axis[0] - s * axis[2], c + tmp[1] * axis[1], tmp[1] * axis[2] + s * axis[0], 0,
                 tmp[2] * axis[0] + s * axis[1], tmp[2] * axis[1] - s * axis[0], c + tmp[2] * axis[2], 0,
                 0, 0, 0, 0);

    const Matrix4<T> r(base.row(0) * m[0][0] + base.row(1) * m[0][1] + base.row(2) * m[0][2],
                 base.row(0) * m[1][0] + base.row(1) * m[1][1] + base.row(2) * m[1][2],
                 base.row(0) * m[2][0] + base.row(1) * m[2][1] + base.row(2) * m[2][2],
                 base.row(3));

    return r;
}

template<typename T, typename X>
auto lerp(const T& a, const T& b, X x) {
    return a + x * (b - a);
    //return a * (X(1) - x) + b * x;
}

template<typename T>
T sign(T t) {
    static_assert(std::is_signed_v<T>);
    if(t == T(0)) {
        return T(0);
    }
    return t > 0.0 ? T(1) : T(-1);
}

template<typename T>
T round_up_to(T val, T to) {
    static_assert(std::is_integral_v<T>);
    const T m = val % to;
    return m ? val + (to - m) : val;
}

template<typename T>
static bool all_finite(const T& t) {
    return std::all_of(t.begin(), t.end(), [](auto x) { return std::isfinite(x); });
}



// https://extremelearning.com.au/unreasonable-effectiveness-of-quasirandom-sequences/#GeneralizingGoldenRatio
template<typename T, typename I>
Vec<2, T> golden_r2_2d(I n) {
    const T g = T(1.32471795724474602596);
    const T a1 = T(1.0) / g;
    const T a2 = T(1.0) / (g * g);
    const T px = T(0.5) + a1 * T(n);
    const T py = T(0.5) + a2 * T(n);

    return Vec<2, T>(std::fmod(px, T(1.0)), std::fmod(py, T(1.0)));
}

// https://www.shadertoy.com/view/4dtBWH
template<typename T, typename I>
Vec<2, T> weyl_2d(I n) {
    const T d24 = T(1 << 24);
    const T px = T(n * 12664745) / d24;
    const T py = T(n * 9560333) / d24;

    return Vec<2, T>(std::fmod(px, T(1.0)), std::fmod(py, T(1.0)));
}

}
}

#endif // Y_MATH_MATH_H

