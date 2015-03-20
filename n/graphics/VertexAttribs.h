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

#ifndef N_GRAPHICS_VERTEXATTRIBS
#define N_GRAPHICS_VERTEXATTRIBS

#include "DynamicBuffer.h"

namespace n {
namespace graphics {

class VertexAttribs
{
	struct Attrib
	{
		uint slot;
		internal::DynamicBufferBase<Array> *buffer;
		gl::GLuint type;
		uint size;
	};

	public:
		VertexAttribs() {
		}

		template<typename T>
		void addAttrib(uint slot, DynamicBuffer<T> *buffer) {
			attribs.append(Attrib({slot, buffer, GLType<T>::value, GLType<T>::size}));
		}

	private:
		template<typename T>
		friend class VertexArrayObject;

		core::Array<Attrib> attribs;
};

}
}


#endif // N_GRAPHICS_VERTEXATTRIBS

