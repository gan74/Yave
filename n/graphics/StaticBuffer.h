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

#ifndef N_GRAPHICS_STATICBUFFER
#define N_GRAPHICS_STATICBUFFER


#include "GL.h"
#include <n/core/AsCollection.h>
#include <n/core/String.h>
#include "GLContext.h"

namespace n {
namespace graphics {

namespace internal {
	inline gl::Handle &getCurrentVao() {
		static gl::Handle current = 0;
		return current;
	}
}

template<typename T, BufferTarget Binding>
class StaticBuffer : NonCopyable
{
	static_assert(TypeInfo<T>::isPod, "StaticBuffer should only contain pod");

	public:
		StaticBuffer(const core::Array<T> &arr) : bufferSize(arr.size()), handle(0) {
			gl::bindVertexArray(internal::getCurrentVao() = 0);
			handle = gl::createBuffer();
			gl::bindBuffer(Binding, handle);
			gl::bufferData(Binding, sizeof(T) * bufferSize, arr.begin(), gl::Static);
		}

		~StaticBuffer() {
			gl::Handle h = handle;
			GLContext::getContext()->addGLTask([=]() {
				gl::deleteBuffer(h);
			});
		}

		void bind() const {
			gl::bindBuffer(Binding, handle);
		}

		uint size() const {
			return bufferSize;
		}

	private:
		uint bufferSize;
		gl::Handle handle;
};

}
}

#endif // N_GRAPHICS_STATICBUFFER

