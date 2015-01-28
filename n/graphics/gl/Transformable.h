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

#ifndef N_GRAPHICS_GL_TRANSFORMABLE
#define N_GRAPHICS_GL_TRANSFORMABLE

#include "GL.h"
#include <n/math/Transform.h>
#ifndef N_NO_GL

namespace n {
namespace graphics {
namespace gl {

template<typename T = float>
class Transformable
{
	public:
		const math::Transform<T> &getTransform() const {
			return transform;
		}

		T getRadius() const {
			return radius;
		}

		const math::Vec<3, T> getPosition() const {
			return transform.getPosition();
		}

		const math::Quaternion<T> getRotation() const {
			return transform.getRotation();
		}

		virtual ~Transformable() {
		}

	protected:
		T radius;
		math::Transform<T> transform;
};

template<typename T = float>
class Movable : public Transformable<T>
{

	public:
		void setPosition(const math::Vec<3, T> &pos) {
			transform = math::Transform<T>(transform.getRotation(), pos);
		}

		void setRotation(const math::Quaternion<T> &q) {
			transform = math::Transform<T>(q, transform.getPosition());
		}

	protected:
		using Transformable<T>::transform;
		using Transformable<T>::radius;
};

}
}
}

#endif
#endif // N_GRAPHICS_GL_TRANSFORMABLE

