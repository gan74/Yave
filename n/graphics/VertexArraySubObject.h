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

#ifndef N_GRAPHICS_VERTEXARRAYSUBOBJECT
#define N_GRAPHICS_VERTEXARRAYSUBOBJECT

#include "VertexArrayObject.h"

namespace n {
namespace graphics {

template<typename T = float>
class VertexArraySubObject
{
	public:
		VertexArraySubObject() : object(0), start(0), size(0), base(0), radius(0) {
		}

		VertexArraySubObject(const core::SmartPtr<VertexArrayObject<T>> &obj) : VertexArraySubObject<T>(obj, 0, obj ? obj->triangleCount() : 0, 0, obj ? obj->getRadius() : 0) {
		}

		VertexArraySubObject(const core::SmartPtr<VertexArrayObject<T>> &obj, uint beg, uint num, uint vertOffset, T rad) : object(obj), start(beg), size(num), base(vertOffset), radius(rad) {
		}

		void draw(const Material &mat, const VertexAttribs &attributes = VertexAttribs(), uint renderFlags = RenderFlag::None, uint instances = 1) const {
			if(object) {
				object->draw(mat, attributes, renderFlags, instances, start, size, base);
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

		bool operator==(const VertexArraySubObject<T> &o) const {
			return object == o.object && start == o.start && size == o.size && base == o.base;
		}

		bool operator<(const VertexArraySubObject<T> &o) const {
			bool obj = object < o.object;
			if(!obj && object == o.object) {
				return start < o.start;
			}
			return obj;
		}

	private:
		core::SmartPtr<VertexArrayObject<T>> object;
		uint start;
		uint size;
		uint base;
		T radius;
};

}
}

#endif // N_GRAPHICS_VERTEXARRAYSUBOBJECT

