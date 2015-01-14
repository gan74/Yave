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

#ifndef N_GRAPHICS_GL_VERTEX
#define N_GRAPHICS_GL_VERTEX

#include <n/math/Vec.h>
#include <n/defines.h>
#ifndef N_NO_GL

namespace n {
namespace graphics {
namespace gl {

template<typename T = float>
class Vertex
{
	public:
		typedef T * iterator;
		typedef const T * const_iterator;

		Vertex(const math::Vec<3, T> &p, const math::Vec<3, T> &n = math::Vec<3, T>(), const math::Vec<2, T> &t = math::Vec<2, T>()) : pos(p), tex(t), norm(n) {
		}

		const math::Vec<3, T> &getPosition() const {
			return pos;
		}

		const math::Vec<3, T> &getNormal() const {
			return norm;
		}

		const math::Vec<2, T> &getTexCoord() const {
			return tex;
		}

		const math::Vec<3, T> &getTangent() const {
			return tang;
		}

		math::Vec<3, T> &getPosition() {
			return pos;
		}

		math::Vec<3, T> &getNormal() {
			return norm;
		}

		math::Vec<2, T> &getTexCoord() {
			return tex;
		}

		math::Vec<3, T> &getTangent() {
			return tang;
		}

		iterator begin() {
			return tex.begin();
		}

		iterator end() {
			return tang.end();
		}

		const_iterator begin() const {
			return tex.begin();
		}

		const_iterator end() const {
			return tang.end();
		}

	private:
		math::Vec<3, T> pos;
		math::Vec<2, T> tex;
		math::Vec<3, T> norm;
		math::Vec<3, T> tang;
};

}
}
}
#endif
#endif // N_GRAPHICS_GL_VERTEX

