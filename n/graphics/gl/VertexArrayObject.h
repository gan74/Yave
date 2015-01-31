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

#ifndef N_GRAPHICS_GL_VERTEXARRAYOBJECT
#define N_GRAPHICS_GL_VERTEXARRAYOBJECT

#include "TriangleBuffer.h"
#include "StaticBuffer.h"
#include <n/defines.h>
#ifndef N_NO_GL

namespace n {
namespace graphics {
namespace gl {

template<typename T = float>
class VertexArrayObject : core::NonCopyable
{
	public:
		VertexArrayObject(const typename TriangleBuffer<T>::FreezedTriangleBuffer &tr) : radius(tr.radius), size(tr.indexes.size() / 3), data(tr.vertices), indexes(tr.indexes), handle(0) {
		}

		void bind() const {
			if(!handle) {
				glGenVertexArrays(1, &handle);
				glBindVertexArray(handle);
				data.bind();
				indexes.bind();
				glVertexAttribPointer(0, 3, GLType<T>::value, GL_FALSE, sizeof(Vertex<T>), 0);
				glVertexAttribPointer(1, 3, GLType<T>::value, GL_FALSE, sizeof(Vertex<T>), (void *)(2 * sizeof(T) + 3 * sizeof(T)));
				glVertexAttribPointer(2, 2, GLType<T>::value, GL_FALSE, sizeof(Vertex<T>), (void *)(2 * sizeof(T)));
				glVertexAttribPointer(3, 3, GLType<T>::value, GL_FALSE, sizeof(Vertex<T>), (void *)(2 * sizeof(T) + 2 * 3 * sizeof(T)));
				glEnableVertexAttribArray(0);
				glEnableVertexAttribArray(1);
				glEnableVertexAttribArray(2);
				glEnableVertexAttribArray(3);
			} else {
				glBindVertexArray(handle);
			}
		}

		void draw(uint instances = 1, uint beg = 0, uint end = 0) const {
			bind();
			if(end <= beg) {
				end = size;
			}
			glDrawElementsInstanced(GL_TRIANGLES, 3 * (end - beg), GLType<uint>::value, (const void *)(3 * beg * sizeof(uint)), instances);
		}

		T getRadius() const {
			return radius;
		}

	private:
		T radius;
		uint size;
		StaticBuffer<Vertex<T>, Array> data;
		StaticBuffer<uint, Index> indexes;

		mutable GLuint handle;
};


}
}
}

#endif

#endif // N_GRAPHICS_GL_VERTEXARRAYOBJECT

