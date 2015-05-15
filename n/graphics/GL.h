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

#include <n/math/Vec.h>

namespace n {
namespace graphics {
namespace gl {

#include <GL/glew.h>
//#include <GL/gl.h>

}

template<typename T>
struct GLType
{
	static constexpr gl::GLenum value = 0;
	static constexpr uint size = 0;
};

template<>
struct GLType<float>
{
	static_assert(sizeof(float) == 4, "float should be 4 byte long.");
	static constexpr gl::GLenum value = GL_FLOAT;
	static constexpr uint size = 1;
};

template<>
struct GLType<double>
{
	static_assert(sizeof(double) == 8, "double should be 8 byte long.");
	static constexpr gl::GLenum value = GL_DOUBLE;
	static constexpr uint size = 1;
};

template<>
struct GLType<int8>
{
	static constexpr gl::GLenum value = GL_BYTE;
	static constexpr uint size = 1;
};

template<>
struct GLType<uint8>
{
	static constexpr gl::GLenum value = GL_UNSIGNED_BYTE;
	static constexpr uint size = 1;
};

template<>
struct GLType<int16>
{
	static constexpr gl::GLenum value = GL_SHORT;
	static constexpr uint size = 1;
};

template<>
struct GLType<uint16>
{
	static constexpr gl::GLenum value = GL_UNSIGNED_SHORT;
	static constexpr uint size = 1;
};

template<>
struct GLType<int32>
{
	static constexpr gl::GLenum value = GL_INT;
	static constexpr uint size = 1;
};

template<>
struct GLType<uint32>
{
	static constexpr gl::GLenum value = GL_UNSIGNED_INT;
	static constexpr uint size = 1;
};

template<uint N, typename T>
struct GLType<math::Vec<N, T>>
{
	static constexpr gl::GLenum value = GLType<T>::value;
	static constexpr uint size = N;
};

enum BufferBinding
{
	Array = GL_ARRAY_BUFFER,
	Index = GL_ELEMENT_ARRAY_BUFFER
};

struct GLTexFormat
{
	GLTexFormat(gl::GLenum f, gl::GLenum i, gl::GLenum t) : format(f), internalFormat(i), type(t) {
	}

	const gl::GLenum format;
	const gl::GLenum internalFormat;
	const gl::GLenum type;
};

}
}
//}

#endif // N_GRAPHICS_GL

