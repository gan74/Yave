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


#include "Texture.h"
#include "GLContext.h"
#include "StaticBuffer.h"
#include "GL.h"

namespace n {
namespace graphics {

Texture::Texture(const Image &i) : image(i), data(0) {
}

Texture::Texture() : data(0) {
}

Texture::~Texture() {
}

bool Texture::operator==(const Texture &t) const {
	return image == t.image;
}

bool Texture::operator!=(const Texture &t) const {
	return !operator==(t);
}

void Texture::bind() const {
	if(!data) {
		if(image.isNull()) {
			gl::glBindTexture(GL_TEXTURE_2D, 0);
		} else {
			if(image.getFormat() != ImageFormat::R8G8B8A8) {
				fatal("Unsuported texture format");
			}
			data = new Data();
			GLContext::getContext()->addGLTask([=]() {
				gl::glGenTextures(1, &(data->handle));
				gl::glBindTexture(GL_TEXTURE_2D, data->handle);
				gl::glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, image.getSize().x(), image.getSize().y());
				gl::glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, image.getSize().x(), image.getSize().y(), GL_RGBA, GL_UNSIGNED_BYTE, image.data());
			});
		}
	} else {
		gl::glBindTexture(GL_TEXTURE_2D, data->handle);
	}
}

}
}
