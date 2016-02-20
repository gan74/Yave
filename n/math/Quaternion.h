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
#ifndef N_MATH_QUATERNION_H
#define N_MATH_QUATERNION_H

#include "Vec.h"

namespace n {
namespace math {

template<typename T = float>
class Quaternion
{

	public:
		static constexpr uint yawIndex = 2;
		static constexpr uint pitchIndex = 1;
		static constexpr uint rollIndex = 0;

		template<typename U>
		Quaternion(const Vec<4, U> &q) : quat(q.normalized()) {
		}

		template<typename... Args>
		Quaternion(Args... args) : Quaternion(Vec<4, T>(args...)) {
		}

		Quaternion() : Quaternion(0, 0, 0, 1) {
		}

		T getAngle() const {
			return std::acos(w() * T(2));
		}

		Vec<3, T> getAxis() const {
			return Vec<3, T>(quat.sub(3) / std::sqrt(T(1) - w() * w()));
		}

		Quaternion<T> inverse() const {
			return Quaternion<T>(-quat.sub(3), quat.w());
		}

		explicit operator Vec<4, T>() const {
			return quat;
		}

		Vec<3, T> operator()(const Vec<3, T> &v) const {
			Vec<3, T> u = quat.sub(3);
			return u * T(2) * u.dot(v)
				  + v * (w() * w() - u.length2())
				  + u.cross(v) * T(2) * w();
		}

		Quaternion<T> operator-() const {
			return inverse();
		}

		Quaternion<T> operator*(T s) const {
			return Quaternion<T>(quat * s);
		}

		Quaternion<T> operator/(T s) const {
			return Quaternion<T>(quat / s);
		}

		template<typename U>
		Quaternion<T> &operator*=(const U &s) const {
			operator=(*this * s);
			return *this;
		}

		template<typename U>
		Quaternion<T> &operator/=(const U &s) const {
			operator=(*this / s);
			return *this;
		}

		template<typename U>
		Quaternion<T> &operator=(const Vec<4, U> &q) {
			quat = q.normalized();
			return *this;
		}

		template<typename U>
		Quaternion<T> &operator=(const Quaternion<U> &q) {
			quat = q.quat;
			return *this;
		}

		Quaternion<T> operator*(const Quaternion<T> &q) const {
			return Quaternion<T>(w() * q.x() + x() * q.w() + y() * q.z() - z() * q.y(),
								 w() * q.y() + y() * q.w() + z() * q.x() - x() * q.z(),
								 w() * q.z() + z() * q.w() + x() * q.y() - y() * q.x(),
								 w() * q.w() - x() * q.x() - y() * q.y() - z() * q.z());
		}

		T x() const {
			 return quat.x();
		}

		T y() const {
			 return quat.y();
		}

		T z() const {
			 return quat.z();
		}

		T w() const {
			 return quat.w();
		}

		Vec<3, T> toEuler() const {
			return Vec<3, T>(std::atan2(T(2) * (w() * x() + y() * z()), T(1) - T(2) * (x() * x() + y() * y())),
							 std::asin(T(2) * (w() * y() - z() * x())),
							 std::atan2(T(2) * (w() * z() + x() * y()), T(1) - T(2) * (y() * y() + z() * z())));
		}

		Vec<4, T> toAxisAngle() const {
			T s = std::sqrt(T(w) - w() * w());
			if(s < epsilon<T>()) {
				s = T(1);
			}
			return Vec<4, T>(quat.sub(3) / s, std::acos(w()) * T(2));
		}

		Vec<3, T> toPacked() const {
			return quat.sub(3);
		}

		static Quaternion<T> fromLookAt(Vec<3, T> f) {
			f.normalize();
			Vec<3, T> axis(1, 0, 0);
			T d = f.dot(axis);
			if(fabs(d + T(1.0)) < math::epsilon<T>()) {
				return fromAxisAngle(math::Vec<3, T>(0, 0, 1), math::pi);
			} else if(fabs(d - T(1.0)) < math::epsilon<T>()) {
				return Quaternion<T>();
			}
			return fromAxisAngle(f.cross(axis), -acos(d));
		}

		static Quaternion<T> fromEuler(T yaw, T pitch, T roll) {
			T cosYaw = std::cos(pitch * T(0.5));
			T sinYaw = std::sin(pitch * T(0.5));
			T cosPitch = std::cos(roll * T(0.5));
			T sinPitch = std::sin(roll * T(0.5));
			T cosRoll = std::cos(yaw * T(0.5));
			T sinRoll = std::sin(yaw * T(0.5));
			return Quaternion<T>(cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw,
								 cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw,
								 sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw,
								 cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw);
		}

		static Quaternion<T> fromBase(const Vec<3, T> &forward, const Vec<3, T> &side, const Vec<3, T> &up) {
			T w = std::sqrt(1 + forward.x() + side.y() + up.z()) * 0.5;
			Vec<3, T> q(side.z() - up.y(), up.x() - forward.z(), forward.y() - side.x());
			return Quaternion<T>(q / (4 * w), w);
		}

		static Quaternion<T> fromEuler(const Vec<3, T> &euler) {
			return fromEuler(euler[yawIndex], euler[pitchIndex], euler[rollIndex]);
		}

		static Quaternion<T> fromAxisAngle(const Vec<3, T> &axis, T ang) {
			T s = std::sin(ang * T(0.5)) / axis.length();
			return Quaternion<T>(axis.x() * s, axis.y() * s, axis.z() * s, std::cos(ang * T(0.5)));
		}

		static Quaternion<T> fromAxisAngle(const Vec<3, T> &axisAng) {
			return fromAxisAngle(axisAng, axisAng.length());
		}

		static Quaternion<T> fromAxisAngle(const Vec<4, T> &v) {
			return fromAxisAngle(v.sub(3), v.w());
		}

		static Quaternion<T> fromPacked(const Vec<3, T> &v) {
			return Quaternion<T>(v, std::sqrt(1 - v.x() * v.x() - v.y() * v.y()));
		}

	private:
		Vec<4, T> quat;
};

}
}

#endif // N_MATH_QUATERNION_H
