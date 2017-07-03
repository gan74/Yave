/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
			PitchIndex = 1,
			RollIndex = 0
		};

		template<typename X>
		Quaternion(const Vec<4, X>& q) : _quat(q.normalized()) {
		}

		template<typename... Args>
		Quaternion(Args&&... args) : _quat(Vec<4, T>(std::forward<Args>(args)...).normalized()) {
		}

		Quaternion(detail::identity_t = identity()) : Quaternion(0, 0, 0, 1) {
		}

		template<typename X>
		Quaternion& operator=(const Vec<4, X>& q) {
			_quat = q.normalized();
			return *this;
		}

		template<typename X>
		Quaternion& operator=(const Quaternion<X>& q) {
			_quat = q._quat;
			return *this;
		}


		T angle() const {
			return std::acos(w() * T(2));
		}

		Vec<3, T> axis() const {
			return Vec<3, T>(_quat.template to<3>() / std::sqrt(T(1) - w() * w()));
		}

		Quaternion inverse() const {
			return Quaternion(-_quat.template to<3>(), _quat.w());
		}

		Vec<3, T> operator()(const Vec<3, T>& v) const {
			Vec<3, T> u = _quat.template to<3>();
			return u * T(2) * u.dot(v) +
				   v * (w() * w() - u.length2()) +
				   u.cross(v) * T(2) * w();
		}

		T x() const {
			 return _quat.x();
		}

		T y() const {
			 return _quat.y();
		}

		T z() const {
			 return _quat.z();
		}

		T w() const {
			 return _quat.w();
		}

		Quaternion lerp(const Quaternion& end_quat, T factor) const {
			Vec<4, T> end = end_quat._quat;
			T consom = _quat.dot(end);

			if(consom < T(0)) {
				end = -end;
				consom = - consom;
			}

			if(T(1) - consom > epsilon<T>) {
				T omega = std::acos(consom);
				T sin = std::sin(omega);
				T p = std::sin((T(1) - factor) * omega) / sin;
				T q = std::sin(factor * omega) / sin;

				return _quat * p + end * q;
			}
			return _quat * (T(1) - factor) + end * factor;
		}

		Vec<3, T> to_euler() const {
			return Vec<3, T>(std::atan2(T(2) * (w() * x() + y() * z()), T(1) - T(2) * (x() * x() + y() * y())),
							 std::asin(T(2) * (w() * y() - z() * x())),
							 std::atan2(T(2) * (w() * z() + x() * y()), T(1) - T(2) * (y() * y() + z() * z())));
		}

		Vec<4, T> to_axis_angle() const {
			T s = std::sqrt(T(w) - w() * w());
			if(s < epsilon<T>()) {
				s = T(1);
			}
			return Vec<4, T>(_quat.template to<3>() / s, std::acos(w()) * T(2));
		}

		Quaternion operator-() const {
			return inverse();
		}

		Quaternion& operator*=(const Quaternion& q) {
			_quat = {w() * q.x() + x() * q.w() + y() * q.z() - z() * q.y(),
					 w() * q.y() + y() * q.w() + z() * q.x() - x() * q.z(),
					 w() * q.z() + z() * q.w() + x() * q.y() - y() * q.x(),
					 w() * q.w() - x() * q.x() - y() * q.y() - z() * q.z()};
			return *this;
		}


		static Quaternion look_at(Vec<3, T> f) {
			f.normalize();
			Vec<3, T> axis(1, 0, 0);
			T d = f.dot(axis);
			if(std::abs(d + T(1.0)) < epsilon<T>()) {
				return from_axis_angle(Vec<3, T>(0, 0, 1), pi<T>);
			} else if(std::abs(d - T(1.0)) < epsilon<T>()) {
				return Quaternion();
			}
			return from_axis_angle(f.cross(axis), -std::acos(d));
		}

		static Quaternion from_euler(T yaw, T pitch, T roll) {
			T cos_yaw = std::cos(pitch * T(0.5));
			T sin_yaw = std::sin(pitch * T(0.5));
			T cos_pitch = std::cos(roll * T(0.5));
			T sin_pitch = std::sin(roll * T(0.5));
			T cos_roll = std::cos(yaw * T(0.5));
			T sin_roll = std::sin(yaw * T(0.5));
			return Quaternion(cos_roll * sin_pitch * cos_yaw + sin_roll * cos_pitch * sin_yaw,
							  cos_roll * cos_pitch * sin_yaw - sin_roll * sin_pitch * cos_yaw,
							  sin_roll * cos_pitch * cos_yaw - cos_roll * sin_pitch * sin_yaw,
							  cos_roll * cos_pitch * cos_yaw + sin_roll * sin_pitch * sin_yaw);
		}

		static Quaternion from_base(const Vec<3, T>& forward, const Vec<3, T>& right, const Vec<3, T>& up) {
			T w = std::sqrt(1 + forward.x() + right.y() + up.z()) * T(0.5);
			Vec<3, T> q(right.z() - up.y(), up.x() - forward.z(), forward.y() - right.x());
			return Quaternion(q / (4 * w), w);
		}

		static Quaternion from_euler(const Vec<3, T>& euler) {
			return fromEuler(euler[YawIndex], euler[PitchIndex], euler[RollIndex]);
		}

		static Quaternion from_axis_angle(const Vec<3, T>& axis, T ang) {
			T s = std::sin(ang * T(0.5)) / axis.length();
			return Quaternion(axis.x() * s, axis.y() * s, axis.z() * s, std::cos(ang * T(0.5)));
		}

		static Quaternion from_axis_angle(const Vec<3, T>& axis_ang) {
			return from_axis_angle(axis_ang, axis_ang.length());
		}

		static Quaternion from_axis_angle(const Vec<4, T>& v) {
			return from_axis_angle(v.sub(3), v.w());
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
