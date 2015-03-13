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

#ifndef N_GRAPHICS_UNIFORMBUFFER
#define N_GRAPHICS_UNIFORMBUFFER

#include "GL.h"
#include <n/core/Ref.h>
#include "GLContext.h"

namespace n {
namespace graphics {

namespace internal {
	class UniformBufferBase : core::NonCopyable
	{
		public:
			UniformBufferBase(uint si) : size(si), buffer(malloc(size)), handle(0), modified(true) {
			}

			~UniformBufferBase() {
				free(buffer);
				if(handle) {
					gl::GLuint *h = new gl::GLuint(handle);
					GLContext::getContext()->addGLTask([=]() {
						gl::glDeleteBuffers(1, h);
						delete h;
					});
				}
			}

		protected:
			friend class graphics::ShaderCombinaison;

			void update() const {
				if(modified) {
					if(!handle) {
						gl::glGenBuffers(1, (gl::GLuint *)&handle);
						gl::glBindBuffer(GL_UNIFORM_BUFFER, handle);
						gl::glBufferData(GL_UNIFORM_BUFFER, size, buffer, GL_DYNAMIC_DRAW);
					} else {
						gl::glBindBuffer(GL_UNIFORM_BUFFER, handle);
						void *p = gl::glMapBuffer(GL_UNIFORM_BUFFER, GL_WRITE_ONLY);
						if(!p) {
							gl::glBufferSubData(GL_UNIFORM_BUFFER, 0, size, buffer);
						} else {
							memcpy(p, buffer, size);
							gl::glUnmapBuffer(GL_UNIFORM_BUFFER);
						}
					}
					modified = false;
				}
			}

			uint size;
			void *buffer;
			gl::GLuint handle;
			mutable bool modified;
	};

}

template<typename T>
class UniformBuffer : public internal::UniformBufferBase
{
	static_assert(TypeInfo<T>::isPod, "UniformBuffer should only contain pod");

	public:
		UniformBuffer(uint si) : internal::UniformBufferBase(si * sizeof(T)) {
		}

		core::Ref<T> operator[](uint i) {
			return core::Ref<T>(((T *)buffer)[i], [=]() { modified = true; });
		}


};

}
}

#endif // N_GRAPHICS_UNIFORMBUFFER

