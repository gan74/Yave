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

#ifndef N_GRAPHICS_TEXTURE_H
#define N_GRAPHICS_TEXTURE_H

#include <n/concurent/SpinLock.h>
#include "Image.h"
#include "GLContext.h"
#include "GL.h"

namespace n {
namespace graphics {

namespace internal {
	class TextureBinding;
}

class Texture
{
	struct Data
	{
		Data() : handle(0) {
		}

		~Data() {
			lock.trylock();
			lock.unlock();
			if(handle) {
				gl::GLuint h = handle;
				GLContext::getContext()->addGLTask([=]() {
					gl::glDeleteTextures(1, &h);
				});
			}
		}

		concurent::SpinLock lock;
		gl::GLuint handle;
	};

	public:
		Texture(const Image &i);
		Texture();
		~Texture();


		bool operator==(const Texture &t) const;
		bool operator!=(const Texture &t) const;
		bool operator<(const Texture &t) const;

		bool isNull() const {
			return !data->handle && image.isNull();
		}

		math::Vec2ui getSize() const {
			return image.getSize();
		}

	private:
		friend class ShaderCombinaison;
		friend class FrameBuffer;
		friend class internal::TextureBinding;

		void prepare(bool sync = true) const;

		gl::GLuint getHandle() const;
		void upload() const;

		Image image;
		mutable core::SmartPtr<Data> data;
};

}
}

#endif // N_GRAPHICS_TEXTURE_H
