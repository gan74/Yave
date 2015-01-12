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

#ifndef N_GRAPHICS_GL_VERTEXBUFFER
#define N_GRAPHICS_GL_VERTEXBUFFER

#include "Vertex.h"
#include <n/defines.h>
#ifndef N_NO_GL

namespace n {
namespace graphics {
namespace gl {

template<typename T = float>
class VertexBuffer
{
	public:
		class Vertex
		{
			public:
			private:
				friend class VertexBuffer;

				Vertex(uint i) : index(i) {
				}

				uint index;
		};

		Vertex append(const gl::Vertex<T> &v) {
			vertices.append(v);
			return vertices.size() - 1;
		}

		const gl::Vertex<T> &operator()(Vertex v) const {
			return vertices[v.index];
		}

		gl::Vertex<T> &operator[](Vertex v) {
			return vertices[v.index];
		}

		const gl::Vertex<T> &operator[](Vertex v) const {
			return vertices[v.index];
		}

		typename core::Array<gl::Vertex<T>>::iterator begin() {
			return vertices.begin();
		}

		typename core::Array<gl::Vertex<T>>::iterator end() {
			return vertices.end();
		}

		typename core::Array<gl::Vertex<T>>::const_iterator begin() const {
			return vertices.begin();
		}

		typename core::Array<gl::Vertex<T>>::const_iterator end() const {
			return vertices.end();
		}

		uint size() const {
			return vertices.size();
		}

	private:
		core::Array<gl::Vertex<T>> vertices;
};

}
}
}

#endif

#endif // N_GRAPHICS_GL_VERTEXBUFFER

