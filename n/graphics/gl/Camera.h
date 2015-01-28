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

#ifndef N_GRAPHICS_GL_CAMERA
#define N_GRAPHICS_GL_CAMERA

#include <n/graphics/gl/Transformable.h>
#include <n/math/Volume.h>
#include <n/utils.h>
#ifndef N_NO_GL

namespace n {
namespace graphics {
namespace gl {

template<typename T = float>
class Camera final : public Transformable<T>, public math::Volume<T>
{
	using Transformable<T>::transform; // WHY ?
	using Transformable<T>::radius; // WHY ?


	public:
		Camera() : far(1000), near(1), ratio(1), fov(math::pi<T>() / 2) {
			radius = far;
			computeViewMatrix();
			computeProjectionMatrix();
		}

		void setPosition(const math::Vec<3, T> &pos) {
			transform = math::Transform<T>(transform.getRotation(), pos);
			view[0][3] = -side.dot(pos);
			view[1][3] = -up.dot(pos);
			view[2][3] = -forward.dot(pos);
		}

		void setRotation(const math::Quaternion<T> &q) {
			transform = math::Transform<T>(q, transform.getPosition());
			computeViewMatrix();
			computeFrustum();
		}

		math::Vec<3, T> getForward() const {
			return -forward;
		}

		void setFov(T f) {
			fov = f;
			computeProjectionMatrix();
			computeFrustum();
		}

		const math::Matrix4<T> &getProjectionMatrix() const {
			return proj;
		}

		const math::Matrix4<T> &getViewMatrix() const {
			return view;
		}

		virtual bool isInside(const math::Vec<3, T> &p, T r) const override {
			math::Vec<3, T> w = (p - transform.getPosition());
			T z = -w.dot(forward);
			return z + r > near && z - r < far
					&& w.dot(frustum[0]) + r > 0
					&& w.dot(frustum[1]) + r > 0
					&& w.dot(frustum[2]) + r > 0
					&& w.dot(frustum[3]) + r > 0;
		}

	private:
		void computeViewMatrix() {
			forward = -transform.getX();
			up = transform.getZ();
			side = -transform.getY();
			math::Vec<3, T> p = transform.getPosition();
			view = math::Matrix4<T>(side.x(), side.y(), side.z(), -side.dot(p),
			up.x(), up.y(), up.z(), -up.dot(p),
			forward.x(), forward.y(), forward.z(), -forward.dot(p),
			0, 0, 0, 1);
		}

		void computeProjectionMatrix() {
			T f = cos(fov / 2.0) / sin(fov / 2.0);
			T z = far - near;
			proj = math::Matrix4<T>(1.0 / (tan(fov / 2) * ratio), 0, 0, 0, 0, f, 0, 0, 0, 0, -(far + near) / z, -1, 0, 0, -(2 * far * near) / z, 0).transposed();
		}

		void computeFrustum() {
			T fovR = fov / 2;
			T hFovR = atan(tan(fovR) * ratio);
			T c = cos(fovR);
			T s = sin(fovR);
			frustum[0] = -forward * s + up * c;
			frustum[1] = -forward * s - up * c;
			c = cos(hFovR);
			s = sin(hFovR);
			frustum[2] = -forward * s + side * c;
			frustum[3] = -forward * s - side * c;
		}


		float far;
		float near;

		float ratio;
		float fov;

		math::Vec<3, T> forward;
		math::Vec<3, T> up;
		math::Vec<3, T> side;

		math::Matrix4<T> view;
		math::Matrix4<T> proj;


		math::Vec<3, T> frustum[4];
};


}
}
}

#endif

#endif // N_GRAPHICS_GL_CAMERA

