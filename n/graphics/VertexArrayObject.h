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
class VertexArrayObject
{
	struct AllocData
	{
		VertexArrayFactory<T> *object;
		uint count;
		uint start;
		uint size;
		uint base;
		T radius;
	};

	public:

		VertexArrayObject() : data{0, 0, 0, 0, 0, 0} {
		}

		~VertexArrayObject() {
		}

		void draw(const Material &mat, const VertexAttribs &attributes = VertexAttribs(), uint renderFlags = RenderFlag::None, uint instances = 1, uint instanceBase = 0) const {
			if(data.object) {
				draw(mat, attributes, renderFlags, instances, data.start, data.size, data.base, instanceBase);
			}
		}

		uint triangleCount() const {
			return data.size;
		}

		bool isNull() const {
			return !data.object;
		}

		T getRadius() const {
			return data.radius;
		}

		gl::DrawCommand getCmd() const {
			return gl::DrawCommand{data.size, 1, data.start, data.base, 0};
		}

		bool operator<(const VertexArrayObject<T> &o) const {
			bool obj = data.object < o.data.object;
			if(!obj && data.object == o.data.object) {
				return data.start < o.data.start;
			}
			return obj;
		}

	private:
		friend class VertexArrayFactory<T>;

		VertexArrayObject(const AllocData &d) : data(d), alloc(new AllocObject(d)) {
		}

		void bind() const;

		AllocData data;

		void draw(const Material &mat, const VertexAttribs &, uint renderFlags, uint instances, uint start, uint tris, uint vertexBase, uint instanceBase) const {
			mat.bind(renderFlags);
			ShaderInstance::validateState();
			ShaderInstance::getCurrent()->setValue(SVBaseInstance, instanceBase);
			bind();
			gl::drawElementsInstancedBaseVertex(gl::Triangles, tris, (void *)(sizeof(uint) * start), instances, vertexBase);
		}

		struct AllocObject
		{
			AllocObject(const AllocData &d) : data(d) {
			}

			~AllocObject() {
				data.object->free(data);
			}

			AllocData data;
		};

		core::SmartPtr<AllocObject> alloc;

};

}
}

#endif // N_GRAPHICS_VERTEXARRAYOBJECT

#include "VertexArrayFactory.h"


#ifndef N_GRAPHICS_VERTEXARRAYOBJECT_IMPL
#define N_GRAPHICS_VERTEXARRAYOBJECT_IMPL

template<typename T>
void n::graphics::VertexArrayObject<T>::bind() const {
	data.object->bind();
}

#endif // N_GRAPHICS_VERTEXARRAYOBJECT_IMPLGL_PATCHES


