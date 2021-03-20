/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#ifndef Y_MATH_QUATERNION_H
#define Y_MATH_QUATERNION_H

#include "math.h"
#include "Vec.h"

#include <limits>

namespace y {
namespace math {

template<typename T = float>
class Quaternion {

    static_assert(std::is_floating_point_v<T>, "Quaternion only support floating point types.");

    public:
        enum Euler {
            YawIndex = 2,
            RollIndex = 1,
            PitchIndex = 0,
        };

        inline constexpr Quaternion(detail::identity_t = identity()) : _quat(0, 0, 0, 1) {
        }

        inline constexpr Quaternion(const Quaternion& q) = default;

        template<typename X>
        inline constexpr Quaternion(const Vec<4, X>& q) : _quat(q.normalized()) {
        }

        template<typename X>
        inline constexpr Quaternion(const Quaternion<X>& q) : _quat(q._quat) {
        }

        template<typename... Args, typename = std::enable_if_t<std::is_constructible_v<Vec<4, T>, Args...>>>
        inline constexpr /*explicit*/ Quaternion(Args&&... args) : _quat(Vec<4, T>(y_fwd(args)...).normalized()) {
        }

        template<typename X>
        inline constexpr Quaternion& operator=(const Quaternion<X>& q) {
            _quat = q._quat;
            return *this;
        }

        template<typename X>
        inline constexpr Quaternion& operator=(const Vec<4, X>& q) {
            _quat = q.normalized();
            return *this;
        }

        inline constexpr bool operator==(const Quaternion& q) const {
            return _quat == q._quat || -_quat == q._quat;
        }

        inline constexpr bool operator!=(const Quaternion& q) const {
            return !operator==(q);
        }

        inline constexpr auto& as_vec() {
            return _quat;
        }

        inline constexpr const auto& as_vec() const {
            return _quat;
        }

        inline constexpr T angle() const {
            return std::acos(w() * T(2.0f));
        }

        inline constexpr Vec<3, T> axis() const {
            return Vec<3, T>(_quat.template to<3>() / std::sqrt(T(1.0f) - w() * w()));
        }

        inline constexpr Quaternion inverse() const {
            return Quaternion(-_quat.template to<3>(), _quat.w());
        }

        inline constexpr Quaternion flip() const {
            return Quaternion(-_quat);
        }

        inline constexpr Vec<3, T> operator()(const Vec<3, T>& v) const {
            Vec<3, T> u = _quat.template to<3>();
            return u * T(2.0f) * u.dot(v) +
                   v * (w() * w() - u.length2()) +
                   u.cross(v) * T(2.0f) * w();
        }

        inline constexpr Vec<4, T> to_axis_angle() const {
            return Vec<4, T>(axis(), angle());
        }

        inline constexpr Quaternion operator-() const {
            return inverse();
        }

        inline constexpr Quaternion& operator*=(const Quaternion& q) {
            _quat = {w() * q.x() + x() * q.w() + y() * q.z() - z() * q.y(),
                     w() * q.y() + y() * q.w() + z() * q.x() - x() * q.z(),
                     w() * q.z() + z() * q.w() + x() * q.y() - y() * q.x(),
                     w() * q.w() - x() * q.x() - y() * q.y() - z() * q.z()};
            return *this;
        }

        inline constexpr T x() const {
             return _quat.x();
        }

        inline constexpr T y() const {
             return _quat.y();
        }

        inline constexpr T z() const {
             return _quat.z();
        }

        inline constexpr T w() const {
             return _quat.w();
        }

        inline constexpr Quaternion slerp(const Quaternion& end_quat, T factor) const {
            Vec<4, T> end = end_quat._quat;
            T dot = _quat.dot(end);

            if(dot < T(0.0f)) {
                end = -end;
                dot = -dot;
            }

            if(T(1.0f) - dot > epsilon<T>) {
                const T omega = std::acos(dot);
                const T sin = std::sin(omega);
                const T p = std::sin((T(1.0f) - factor) * omega) / sin;
                const T q = std::sin(factor * omega) / sin;

                return _quat * p + end * q;
            }
            return _quat * (T(1.0f) - factor) + end * factor;
        }

        inline constexpr Quaternion lerp(const Quaternion& end_quat, T factor) const {
            Vec<4, T> end = end_quat._quat;
            T dot = _quat.dot(end);

            if(dot < T(0.0f)) {
                end = -end;
                dot = -dot;
            }

            return _quat * (T(1.0f) - factor) + end * factor;
        }

        inline constexpr T pitch() const {
            //return T(atan(T(2.0f) * (y * z + w * x), w * w - x * x - y * y + z * z));
            const T a = T(2.0f) * (y() * z() + w() * x());
            const T b = w() * w() - x() * x() - y() * y() + z() * z();
            if(b == 0 && a == 0) {
                return T(2.0f) * std::atan2(x(), w());
            }
            return std::atan2(a, b);
        }

        inline constexpr T yaw() const {
            return std::atan2(T(2.0f) * (x() * y() + w() * z()), w() * w() + x() * x() - y() * y() - z() * z());
        }

        inline constexpr T roll() const {
            const T a = T(-2.0f) * (x() * z() - w() * y());
            return std::asin(std::min(std::max(a, T(-1.0f)), T(1.0f)));
        }

        inline constexpr Vec<3, T> to_euler() const {
            Vec<3, T> v;
            v[PitchIndex] = pitch();
            v[YawIndex] = yaw();
            v[RollIndex] = roll();
            return v;
        }


        static Quaternion look_at(Vec<3, T> f) {
            f.normalize();
            const Vec<3, T> axis(1, 0, 0);
            const T d = f.dot(axis);
            if(std::abs(d + T(1.0f)) < epsilon<T>()) {
                return from_axis_angle(Vec<3, T>(0, 0, 1), pi<T>);
            } else if(std::abs(d - T(1.0f)) < epsilon<T>()) {
                return Quaternion();
            }
            return from_axis_angle(f.cross(axis), -std::acos(d));
        }

        static Quaternion from_euler(T pitch, T yaw, T roll) {
            // from https://github.com/g-truc/glm/blob/master/glm/gtc/quaternion.inl#L173

            // swizzle of doom
            const Vec<3, T> c(std::cos(pitch * T(0.5)), std::cos(roll * T(0.5)), std::cos(yaw * T(0.5)));
            const Vec<3, T> s(std::sin(pitch * T(0.5)), std::sin(roll * T(0.5)), std::sin(yaw * T(0.5)));
            Vec<4, T> q;
            q.x() = s.x() * c.y() * c.z() - c.x() * s.y() * s.z();
            q.y() = c.x() * s.y() * c.z() + s.x() * c.y() * s.z();
            q.z() = c.x() * c.y() * s.z() - s.x() * s.y() * c.z();
            q.w() = c.x() * c.y() * c.z() + s.x() * s.y() * s.z();
            return q.normalized();
        }

        static Quaternion from_base(const Vec<3, T>& forward, const Vec<3, T>& left, const Vec<3, T>& up) {
            // from https://github.com/g-truc/glm/blob/master/glm/gtc/quaternion.inl#L650
            const T wxyz[4] = {
                    forward.x() + left.y() + up.z(),
                    forward.x() - left.y() - up.z(),
                    left.y() - forward.x() - up.z(),
                    up.z() - forward.x() - left.y()
                };
            T max = wxyz[0];
            usize max_index = 0;
            for(usize i = 1; i != 4; ++i) {
                if(max < wxyz[i]) {
                    max = wxyz[i];
                    max_index = i;
                }
            }
            const T s = std::sqrt(max + T(1.0f)) * T(0.5);
            const T m = T(0.25) / s;
            switch(max_index) {
                case 0: // w
                    return Quaternion((left[2] - up[1]) * m, (up[0] - forward[2]) * m, (forward[1] - left[0]) * m, s);
                case 1: // x
                    return Quaternion(s, (forward[1] + left[0]) * m, (up[0] + forward[2]) * m, (left[2] - up[1]) * m);
                case 2: // y
                    return Quaternion((forward[1] + left[0]) * m, s, (left[2] + up[1]) * m, (up[0] - forward[2]) * m);
                default: // z
                    return Quaternion((up[0] + forward[2]) * m, (left[2] + up[1]) * m, s, (forward[1] - left[0]) * m);
            }
            return Quaternion();
        }



        inline constexpr static Quaternion from_euler(const Vec<3, T>& euler) {
            return from_euler(euler[PitchIndex], euler[YawIndex], euler[RollIndex]);
        }

        inline constexpr static Quaternion from_axis_angle(const Vec<3, T>& axis, T ang) {
            T s = std::sin(ang * T(0.5)) / axis.length();
            return Quaternion(axis.x() * s, axis.y() * s, axis.z() * s, std::cos(ang * T(0.5)));
        }

        inline constexpr static Quaternion from_axis_angle(const Vec<3, T>& axis_ang) {
            return from_axis_angle(axis_ang, axis_ang.length());
        }

        inline constexpr static Quaternion from_axis_angle(const Vec<4, T>& v) {
            return from_axis_angle(v.template to<3>(), v.w());
        }

    private:
        Vec<4, T> _quat;
};


template<typename T, typename R>
auto operator*(Quaternion<T> v, const R& r) {
    v *= r;
    return v;
}


static_assert(std::is_trivially_copyable_v<Quaternion<>>, "Quaternion<T> should be trivially copyable");

}
}

#endif // Y_MATH_QUATERNION_H

