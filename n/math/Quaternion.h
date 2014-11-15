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

#ifndef N_MATH_QUATERNION_H
#define N_MATH_QUATERNION_H

#include "Vec.h"

namespace n {
namespace math {

template<typename T>
class Quaternion
{

	public:
		static constexpr uint yawIndex = 2;
		static constexpr uint pitchIndex = 1;
		static constexpr uint rollIndex = 0;


		template<typename U>
		Quaternion(const Vec<U, 4> &q) : quat(q.normalized()) {
		}

		template<typename... Args>
		Quaternion(Args... args) : Quaternion(Vec<T, 4>(args...)) {
		}

		Quaternion() : Quaternion(0, 0, 0, 1) {
		}

		T getAngle() const {
			return acos(w() * T(2));
		}

		Vec<T, 3> getAxis() const {
			return Vec<T, 3>(quat.sub(3) /sqrt(T(1) - w() * w()));
		}

		Quaternion<T> inverse() const {
			return Quaternion<T>(-quat.sub(3), quat.w());
		}

		operator Vec<T, 4>() const {
			return quat;
		}

		Vec<T, 3> operator()(const Vec<T, 3> &v) const {
			Vec<T, 3> u = quat.sub(3);
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
		Quaternion<T> &operator=(const Vec<U, 4> &q) {
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

		Vec<T, 3> toEuler() const {
			return Vec<T, 3>(atan2(T(2) * (w() * x() + y() * z()), T(1) - T(2) * (x() * x() + y() * y())),
							 wasin(T(2) * (w() * y() - z() * x())),
							 atan2(T(2) * (w() * z() + x() * y()), T(1) - T(2) * (y() * y() + z() * z())));
		}

		Vec<T, 4> toAxisAngle() const {
			T s = sqrt(T(w) - w() * w());
			if(s < epsilon<T>()) {
				s = T(1);
			}
			return Vec<T, 4>(quat.sub(3) / s, acos(w()) * T(2));
		}

		static Quaternion<T> fromEuler(T yaw, T pitch, T roll) {
			T cosYaw = cos(yaw * T(0.5));
			T sinYaw = sin(yaw * T(0.5));
			T cosPitch = cos(pitch * T(0.5));
			T sinPitch = sin(pitch * T(0.5));
			T cosRoll = cos(roll * T(0.5));
			T sinRoll = sin(roll * T(0.5));
			return Quaternion<T>(cosRoll * sinPitch * cosYaw + sinRoll * cosPitch * sinYaw,
								 cosRoll * cosPitch * sinYaw - sinRoll * sinPitch * cosYaw,
								 sinRoll * cosPitch * cosYaw - cosRoll * sinPitch * sinYaw,
								 cosRoll * cosPitch * cosYaw + sinRoll * sinPitch * sinYaw);
		}

		static Quaternion<T> fromEuler(const Vec<T, 3> &euler) {
			return fromEuler(euler[yawIndex], euler[pitchIndex], euler[rollIndex]);
		}

		static Quaternion<T> fromAxisAngle(const Vec<T, 3> &axis, T ang) {
			T s = sin(ang * T(0.5)) / axis.length();
			return Quaternion<T>(axis.x() * s, axis.y() * s, axis.z() * s, cos(ang * T(0.5)));
		}

		static Quaternion<T> fromAxisAngle(const Vec<T, 4> &v) {
			return fromAxisAngle(v.sub(3), v.w());
		}

	private:
		Vec<T, 4> quat;
};

}
}

#endif // N_MATH_QUATERNION_H
