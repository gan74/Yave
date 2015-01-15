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


#include <n/defines.h>
#ifndef N_NO_GL

#include <iostream>
#include "Texture.h"
#include "Context.h"
#include "Buffer.h"
#include "GL.h"

namespace n {
namespace graphics {
namespace gl {



Texture::Texture(const Image &i) : image(i), data(0) {
}

Texture::~Texture() {
}


void Texture::bind() const {
	if(!data) {
		if(image.isNull()) {
			glBindTexture(GL_TEXTURE_2D, 0);
		} else {
			if(image.getFormat() != ImageFormat::R8G8B8A8) {
				fatal("Unsuported texture format");
			}
			data = new Data();
			Context::getContext()->addGLTask([=]() {
				glGenTextures(1, &(data->handle));
				glBindTexture(GL_TEXTURE_2D, data->handle);
				glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, image.getSize().x(), image.getSize().y());
				glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image.getSize().x(), image.getSize().y(), GL_RGBA, GL_UNSIGNED_BYTE, image.data());
			});
		}
	} else {
		glBindTexture(GL_TEXTURE_2D, data->handle);
	}
}

}
}
}

#endif
