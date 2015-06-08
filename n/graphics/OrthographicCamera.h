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

#ifndef N_GRAPHICS_ORTHOGRAPHICCAMERA
#define N_GRAPHICS_ORTHOGRAPHICCAMERA

#include "Camera.h"

namespace n {
namespace graphics {

template<typename T = float>
class OrthographicCamera final : public Camera<T>
{
	protected:
		using Transformable<T>::transform;
		using Transformable<T>::radius;
		using Camera<T>::view;
		using Camera<T>::proj;

	public:
		OrthographicCamera(const math::Vec<3, T> &s) : Camera<T>(), size(s) {
			radius = size.length();
			computeVectors();
			computeViewMatrix();
			computeProjectionMatrix();
		}

		void setPosition(const math::Vec<3, T> &pos) {
			transform = math::Transform<T>(transform.getRotation(), pos);
			view[0][3] = side.dot(pos);
			view[1][3] = -up.dot(pos);
			view[2][3] = forward.dot(pos);
		}

		void setRotation(const math::Quaternion<T> &q) {
			transform = math::Transform<T>(q, transform.getPosition());
			computeVectors();
			computeViewMatrix();
		}

		void setSize(const math::Vec<3, T> &s) {
			size = s;
			computeVectors();
			computeProjectionMatrix();
		}

		const math::Vec<3, T> & getSize() const {
			return size;
		}

		const math::Matrix4<T> &getProjectionMatrix() const {
			return proj;
		}

		const math::Matrix4<T> &getViewMatrix() const {
			return view;
		}

		virtual bool isInside(const math::Vec<3, T> &p, T r) const override {
			math::Vec<3, T> w = (p - transform.getPosition());
			return fabs(w.dot(forward)) < size.x() + r &&
				   fabs(w.dot(side)) < size.y() + r &&
				   fabs(w.dot(up)) < size.z() + r;
		}

		void setForward(math::Vec<3, T> f, typename Camera<T>::RotationAxis r = Camera<T>::Up) {
			f.normalize();
			if(fabs(f.dot(forward)) == 1.0) {
				return;
			}
			math::Vec<3, T> s = (r == Camera<T>::Side ? math::Vec<3, T>(0, -1, 0) : math::Vec<3, T>(0, 0, -1)) ^ f;
			math::Vec<3, T> u = (s ^ f).normalized();
			s = (u ^ f).normalized();
			setRotation(math::Quaternion<T>::fromBase(f, s, u));
		}

		const math::Vec<3, T> &getForward() const {
			return forward;
		}

	private:
		void computeVectors() {
			forward = transform.getX();
			side = transform.getY();
			up = transform.getZ();
		}

		void computeViewMatrix() {
			math::Vec<3, T> p = transform.getPosition();
			view = math::Matrix4<T>(-side, side.dot(p),
									up, -up.dot(p),
									-forward, forward.dot(p),
			0, 0, 0, 1);
		}

		void computeProjectionMatrix() {
			T right = size.y();
			T left = -right;
			T top = size.z();
			T bottom = -top;
			T zFar = size.x();
			T zNear = -zFar;
			proj = math::Matrix4<T>(T(2) / (right - left), 0, 0, -(right + left) / (right - left),
									0, T(2) / (top - bottom), 0, -(top + bottom) / (top - bottom),
									0, 0, -T(2) / (zFar - zNear), -(zFar + zNear) / (zFar - zNear),
									0, 0, 0, T(1));
		}


		math::Vec<3, T> forward;
		math::Vec<3, T> up;
		math::Vec<3, T> side;

		math::Vec<3, T> size;


};

}
}

#endif // N_GRAPHICS_ORTHOGRAPHICCAMERA

