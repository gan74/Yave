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

#ifndef N_GRAPHICS_GL_STATICBUFFER
#define N_GRAPHICS_GL_STATICBUFFER


#include "GL.h"
#include <n/core/AsCollection.h>
#include <n/core/String.h>
#include <n/core/Ref.h>
#ifndef N_NO_GL

namespace n {
namespace graphics {
namespace gl {

template<typename T, BufferBinding Binding>
class StaticBuffer : core::NonCopyable
{
	static_assert(TypeInfo<T>::isPod, "StaticBuffer should only contain pod");

	public:
		StaticBuffer(const core::Array<T> &arr) : bufferSize(arr.size()), buffer(malloc(sizeof(T) * bufferSize)), handle(0) {
			memcpy(buffer, arr.begin(), bufferSize * sizeof(T));
			setUp();
		}

		~StaticBuffer() {
			if(buffer) {
				free(buffer);
			}
			if(handle) {
				glDeleteBuffers(1, &handle);
			}
		}

		void bind() {
			glBindBuffer(Binding, handle);
		}

		uint size() const {
			return bufferSize;
		}

	private:
		void setUp() {
			if(!handle) {
				glGenBuffers(1, &handle);
				glBindBuffer(Binding, handle);
				glBufferData(Binding, sizeof(T) * bufferSize, buffer, GL_STATIC_DRAW);
			} else {
				glBindBuffer(Binding, handle);
				void *p = glMapBuffer(Binding, GL_WRITE_ONLY);
				if(!p) {
					glBufferSubData(Binding, 0, sizeof(T) * bufferSize, buffer);
				} else {
					memcpy(p, buffer, sizeof(T) * bufferSize);
					glUnmapBuffer(Binding);
				}
			}
		}

		uint bufferSize;
		void *buffer;

		GLuint handle;
};

}
}
}

#endif

#endif // N_GRAPHICS_GL_STATICBUFFER

