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

#ifndef N_GRAPHICS_GL
#define N_GRAPHICS_GL

#include <n/defines.h>
#include <n/types.h>

namespace n {
namespace graphics {
namespace gl {

#include <GL/glew.h>
#include <GL/gl.h>

}

template<typename T>
struct GLType
{
	static constexpr gl::GLenum value = 0;
};

template<>
struct GLType<float>
{
	static_assert(sizeof(float) == 4, "float should be 4 byte long.");
	static constexpr gl::GLenum value = GL_FLOAT;
};

template<>
struct GLType<double>
{
	static_assert(sizeof(double) == 8, "double should be 8 byte long.");
	static constexpr gl::GLenum value = GL_DOUBLE;
};

template<>
struct GLType<int8>
{
	static constexpr gl::GLenum value = GL_BYTE;
};

template<>
struct GLType<uint8>
{
	static constexpr gl::GLenum value = GL_UNSIGNED_BYTE;
};

template<>
struct GLType<int16>
{
	static constexpr gl::GLenum value = GL_SHORT;
};

template<>
struct GLType<uint16>
{
	static constexpr gl::GLenum value = GL_UNSIGNED_SHORT;
};

template<>
struct GLType<int32>
{
	static constexpr gl::GLenum value = GL_INT;
};

template<>
struct GLType<uint32>
{
	static constexpr gl::GLenum value = GL_UNSIGNED_INT;
};

enum BufferBinding
{
	Array = GL_ARRAY_BUFFER,
	Index = GL_ELEMENT_ARRAY_BUFFER
};

}
}
//}

#endif // N_GRAPHICS_GL

