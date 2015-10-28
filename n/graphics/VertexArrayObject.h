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

#ifndef N_GRAPHICS_VERTEXARRAYOBJECT
#define N_GRAPHICS_VERTEXARRAYOBJECT

#include "TriangleBuffer.h"
#include "StaticBuffer.h"
#include "VertexAttribs.h"
#include "Material.h"
#include "ShaderInstance.h"

#include <iostream>

namespace n {
namespace graphics {

template<typename T = float>
class VertexArrayObject : NonCopyable
{
	public:
		VertexArrayObject(const typename TriangleBuffer<T>::FreezedTriangleBuffer &tr) : VertexArrayObject(new typename TriangleBuffer<T>::FreezedTriangleBuffer(tr)) {
		}

		VertexArrayObject(const core::SmartPtr<typename TriangleBuffer<T>::FreezedTriangleBuffer> &tr) :radius(tr->radius), size(tr->indexes.size() % 3 ? uint(fatal("Invalid non-trianglulated mesh.")) : tr->indexes.size() / 3), buffer(tr), data(0), indexes(0), handle(0) {
		}

		~VertexArrayObject() {
			delete data;
			delete indexes;
			if(handle) {
				gl::Handle h = handle;
				GLContext::getContext()->addGLTask([=]() {
					gl::deleteVertexArray(h);
				});
			}
		}

		void draw(const Material &mat, const VertexAttribs &attributes = VertexAttribs(), uint renderFlags = RenderFlag::None, uint instances = 1) const {
			draw(mat, attributes, renderFlags, instances, 0, size, 0, 0);
		}

		void draw(const Material &mat, const VertexAttribs &attributes, uint renderFlags, uint instances, uint start, uint tris, uint vertexBase, uint instanceBase) const {
			mat.bind(renderFlags);
			bind();
			bindAttribs(attributes);
			ShaderInstance::validateState();
			ShaderInstance::getCurrent()->setValue(SVBaseInstance, instanceBase);
			gl::drawElementsInstancedBaseVertex(gl::Triangles, 3 * tris, GLType<uint>::value, (void *)(sizeof(uint) * 3 * start), instances, vertexBase);
		}

		T getRadius() const {
			return radius;
		}

		uint triangleCount() const {
			return size;
		}

	private:
		static uint maxAttribs() {
			return GLContext::getContext()->getHWInt(GLContext::MaxVertexAttribs) - 4;
		}

		void bindAttribs(const VertexAttribs &) const {
		}

		void bind() const {
			if(!handle) {
				data = new StaticBuffer<Vertex<T>, ArrayBuffer>(buffer->vertices);
				indexes = new StaticBuffer<uint, IndexBuffer>(buffer->indexes);
				buffer = 0;

				data->bind();
				handle = gl::createVertexArray();
				gl::bindVertexArray(internal::getCurrentVao() = handle);
				indexes->bind();
				gl::vertexAttribPointer(0, 3, GLType<T>::value, false, sizeof(Vertex<T>), 0);
				gl::vertexAttribPointer(1, 3, GLType<T>::value, false, sizeof(Vertex<T>), (void *)(2 * sizeof(T) + 3 * sizeof(T)));
				gl::vertexAttribPointer(2, 3, GLType<T>::value, false, sizeof(Vertex<T>), (void *)(2 * sizeof(T) + 2 * 3 * sizeof(T)));
				gl::vertexAttribPointer(3, 2, GLType<T>::value, false, sizeof(Vertex<T>), (void *)(3 * sizeof(T)));
				gl::enableVertexAttribArray(0);
				gl::enableVertexAttribArray(1);
				gl::enableVertexAttribArray(2);
				gl::enableVertexAttribArray(3);
			} else {
				if(internal::getCurrentVao() != handle) {
					gl::bindVertexArray(internal::getCurrentVao() = handle);
				}
			}
		}


		T radius;
		uint size;

		mutable core::SmartPtr<typename TriangleBuffer<>::FreezedTriangleBuffer> buffer;
		mutable StaticBuffer<Vertex<T>, ArrayBuffer> *data;
		mutable StaticBuffer<uint, IndexBuffer> *indexes;

		mutable gl::Handle handle;
};

}
}

#endif // N_GRAPHICS_VERTEXARRAYOBJECT

