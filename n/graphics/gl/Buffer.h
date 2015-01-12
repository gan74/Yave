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

#ifndef N_GRAPHICS_GL_BUFFER
#define N_GRAPHICS_GL_BUFFER

#include <iostream>
#include "GL.h"
#include <n/core/String.h>
#include <n/core/Ref.h>
#ifndef N_NO_GL

namespace n {
namespace graphics {
namespace gl {

template<typename T, BufferBinding Binding>
class Buffer : core::NonCopyable
{
	public:
		Buffer(uint s, const T *d = 0) : bufferSize(s), buffer(new T[bufferSize]), modified(true), handle(0) {
			fill(d);
		}

		void fill(const T *d) {
			if(d) {
				memcpy(buffer, d, bufferSize * sizeof(T));
			}
			modified = true;
		}

		core::Ref<T> operator[](uint i) {
			return core::Ref<T>(buffer[i], [=]() { modified = true; });
		}

		const core::Ref<T> operator[](uint i) const {
			return core::Ref<T>(buffer[i], [=]() { modified = true; });
		}

		bool isModified() {
			return modified;
		}

		void bind() {
			if(modified) {
				update();
			} else {
				glBindBuffer(Binding, handle);
			}
		}

		uint size() const {
			return bufferSize;
		}

	private:
		void update() {
			if(!handle) {
				glGenBuffers(1, &handle);
				glBindBuffer(Binding, handle);
				glBufferData(Binding, sizeof(T) * bufferSize, buffer, getBufferUsage(Binding));
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
			modified = false;
		}

		uint bufferSize;
		T *buffer;
		bool modified;

		GLuint handle;
};

}
}
}

#endif

#endif // N_GRAPHICS_GL_BUFFER

