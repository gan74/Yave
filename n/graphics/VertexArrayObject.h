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


#include "Material.h"
#include "VertexAttribs.h"
#include "ShaderInstance.h"
#include "TriangleBuffer.h"

namespace n {
namespace graphics {

template<typename T>
class VertexPool;

template<typename T>
class VertexArrayObject
{
	public:
		VertexArrayObject() : object(0), start(0), size(0), base(0), radius(0) {
		}

		void draw(const Material &mat, const VertexAttribs &attributes = VertexAttribs(), uint renderFlags = RenderFlag::None, uint instances = 1, uint instanceBase = 0) const {
			if(object) {
				draw(mat, attributes, renderFlags, instances, start, size, base, instanceBase);
			}
		}

		uint triangleCount() const {
			return size;
		}

		bool isNull() const {
			return !object;
		}

		T getRadius() const {
			return radius;
		}

		VertexArrayObject<T> &operator=(const VertexArrayObject<T> &o) {
			object = o.object;
			start = o.start;
			size = o.size;
			base = o.base;
			radius = o.radius;
			return *this;
		}

		bool operator==(const VertexArrayObject<T> &o) const {
			return object == o.object && start == o.start && size == o.size && base == o.base;
		}

		bool operator<(const VertexArrayObject<T> &o) const {
			bool obj = object < o.object;
			if(!obj && object == o.object) {
				return start < o.start;
			}
			return obj;
		}

	private:
		void draw(const Material &mat, const VertexAttribs &, uint renderFlags, uint instances, uint start, uint tris, uint vertexBase, uint instanceBase) const {
			mat.bind(renderFlags);
			ShaderInstance::validateState();
			ShaderInstance::getCurrent()->setValue(SVBaseInstance, instanceBase);
			bind();
			gl::drawElementsInstancedBaseVertex(gl::Triangles, 3 * tris, GLType<uint>::value, (void *)(sizeof(uint) * 3 * start), instances, vertexBase);
		}

		void bind() const;

		friend class VertexArrayFactory<T>;

		VertexArrayFactory<T> *object;
		uint start;
		uint size;
		uint base;
		T radius;
};

}
}

#endif // N_GRAPHICS_VERTEXARRAYOBJECT

