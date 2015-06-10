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

class OrthographicCamera final : public Camera
{
	public:
		OrthographicCamera(const math::Vec3 &s) : Camera(), size(s) {
			radius = size.length();
			computeVectors();
			computeViewMatrix();
			computeProjectionMatrix();
		}

		void setPosition(const math::Vec3 &pos) {
			transform = math::Transform<>(transform.getRotation(), pos);
			view[0][3] = side.dot(pos);
			view[1][3] = -up.dot(pos);
			view[2][3] = forward.dot(pos);
		}

		void setRotation(const math::Quaternion<> &q) {
			transform = math::Transform<>(q, transform.getPosition());
			computeVectors();
			computeViewMatrix();
		}

		void setSize(const math::Vec3 &s) {
			size = s;
			computeVectors();
			computeProjectionMatrix();
		}

		const math::Vec3 &getSize() const {
			return size;
		}

		const math::Matrix4<> &getProjectionMatrix() const {
			return proj;
		}

		const math::Matrix4<> &getViewMatrix() const {
			return view;
		}

		virtual bool isInside(const math::Vec3 &p, float r) const override {
			math::Vec3 w = (p - transform.getPosition());
			return fabs(w.dot(forward)) < size.x() + r &&
				   fabs(w.dot(side)) < size.y() + r &&
				   fabs(w.dot(up)) < size.z() + r;
		}

		void setForward(math::Vec3 f, Camera::RotationAxis r = Camera::Up) {
			f.normalize();
			if(fabs(f.dot(forward)) == 1.0) {
				return;
			}
			math::Vec3 s = (r == Camera::Side ? math::Vec3(0, -1, 0) : math::Vec3(0, 0, -1)) ^ f;
			math::Vec3 u = (s ^ f).normalized();
			s = (u ^ f).normalized();
			setRotation(math::Quaternion<>::fromBase(f, s, u));
		}

		const math::Vec3 &getForward() const {
			return forward;
		}

	private:
		void computeVectors() {
			forward = transform.getX();
			side = transform.getY();
			up = transform.getZ();
		}

		void computeViewMatrix() {
			math::Vec3 p = transform.getPosition();
			view = math::Matrix4<>(-side, side.dot(p),
									up, -up.dot(p),
									-forward, forward.dot(p),
			0, 0, 0, 1);
		}

		void computeProjectionMatrix() {
			float right = size.y();
			float left = -right;
			float top = size.z();
			float bottom = -top;
			float zFar = size.x();
			float zNear = -zFar;
			proj = math::Matrix4<>((2) / (right - left), 0, 0, -(right + left) / (right - left),
									0, (2) / (top - bottom), 0, -(top + bottom) / (top - bottom),
									0, 0, -(2) / (zFar - zNear), -(zFar + zNear) / (zFar - zNear),
									0, 0, 0, (1));
		}


		math::Vec3 forward;
		math::Vec3 up;
		math::Vec3 side;

		math::Vec3 size;


};

}
}

#endif // N_GRAPHICS_ORTHOGRAPHICCAMERA

