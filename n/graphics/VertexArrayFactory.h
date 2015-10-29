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

#ifndef N_GRAPHICS_VERTEXARRAYFACTORY
#define N_GRAPHICS_VERTEXARRAYFACTORY

#include "UniformBuffer.h"
#include "Vertex.h"
#include "VertexArrayObject.h"
#include <n/core/Timer.h>
#include <iostream>

namespace n {
namespace graphics {


class VertexArrayBase
{
	private:
		template<typename T>
		friend class VertexArrayFactory;
		template<typename T, BufferTarget Binding>
		friend class StaticBuffer;
		friend class DynamicBufferBase;

		static gl::Handle &currentVao() {
			static gl::Handle current = 0;
			return current;
		}
};

template<typename T>
class VertexArrayFactory : NonCopyable
{
	public:
		static constexpr uint DefaultSize = 1024 * 1024;

		VertexArrayFactory() : vertices(DefaultSize), indexes(DefaultSize), handle(0), verticesUsed(0), indexesUsed(0) {
			setup();
		}

		VertexArrayObject<T> operator()(const typename TriangleBuffer<T>::FreezedTriangleBuffer &buff) {
			mutex.lock();
			VertexArrayObject<T> vao;
			vao.object = this;
			vao.radius = buff.radius;
			vao.size = buff.indexes.size() / 3;
			vao.start = indexesUsed / 3;
			vao.base = verticesUsed;

			for(const Vertex<T> &b : buff.vertices) {
				vertices[verticesUsed++] = b;
			}
			for(uint b : buff.indexes) {
				indexes[indexesUsed++] = b;
			}
			mutex.unlock();
			return vao;
		}


	private:
		friend class VertexArrayObject<T>;

		TypedDynamicBuffer<Vertex<T>, ArrayBuffer> vertices;
		TypedDynamicBuffer<uint, IndexBuffer> indexes;
		gl::Handle handle;

		uint verticesUsed;
		uint indexesUsed;

		concurrent::Mutex mutex;

		void setup() {
			handle = gl::createVertexArray();
			gl::bindVertexArray(VertexArrayBase::currentVao() = handle);
			vertices.update(true);
			indexes.update(true);
			gl::vertexAttribPointer(0, 3, GLType<T>::value, false, sizeof(Vertex<T>), 0);
			gl::vertexAttribPointer(1, 3, GLType<T>::value, false, sizeof(Vertex<T>), (void *)(2 * sizeof(T) + 3 * sizeof(T)));
			gl::vertexAttribPointer(2, 3, GLType<T>::value, false, sizeof(Vertex<T>), (void *)(2 * sizeof(T) + 2 * 3 * sizeof(T)));
			gl::vertexAttribPointer(3, 2, GLType<T>::value, false, sizeof(Vertex<T>), (void *)(3 * sizeof(T)));
			gl::enableVertexAttribArray(0);
			gl::enableVertexAttribArray(1);
			gl::enableVertexAttribArray(2);
			gl::enableVertexAttribArray(3);
		}

		void bind() {
			if(VertexArrayBase::currentVao() != handle) {
				if(vertices.isModified()) {
					mutex.lock();
					core::Timer timer;
					vertices.update();
					indexes.update();
					std::cout<<timer.elapsed()<<"s"<<std::endl;
					mutex.unlock();
				}
				gl::bindVertexArray(VertexArrayBase::currentVao() = handle);
			}
		}




};

template<typename T>
void VertexArrayObject<T>::bind() const {
	object->bind();
}



}
}

#endif // N_GRAPHICS_VERTEXARRAYFACTORY

