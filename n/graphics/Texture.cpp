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
#include "TextureBinding.h"

#define N_NO_TEX_STORAGE

namespace n {
namespace graphics {

struct GLTexFormat
{
	GLTexFormat(gl::GLenum f, gl::GLenum i, gl::GLenum t) : format(f), internalFormat(i), type(t) {
	}

	const gl::GLenum format;
	const gl::GLenum internalFormat;
	const gl::GLenum type;
};

GLTexFormat getTextureFormat(ImageFormat format) {
	switch(format) {
		case ImageFormat::Depth32:
			return GLTexFormat(GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT32, GL_FLOAT);
		break;
		case ImageFormat::R8G8B8A8:
			return GLTexFormat(GL_RGBA, GL_RGBA8, GL_UNSIGNED_BYTE);
		break;
		case ImageFormat::R16G16B16A16:
			return GLTexFormat(GL_RGBA, GL_RGBA16, GL_UNSIGNED_SHORT);
		break;
		default:
			return fatal("Unsuported texture format.");
		break;
	}
}

Texture::Texture(const Image &i) : image(i), data(new Data()) {
}

Texture::Texture() : data(new Data()) {
}

Texture::~Texture() {
}

bool Texture::operator==(const Texture &t) const {
	return image == t.image;
}

bool Texture::operator<(const Texture &t) const {
	return image < t.image;
}

bool Texture::operator!=(const Texture &t) const {
	return !operator==(t);
}

void Texture::upload() const {
	math::Vec2ui size = image.getSize();
	if(!size.mul()) {
		fatal("Invalid image size.");
	}
	gl::glGenTextures(1, &(data->handle));
	gl::glBindTexture(GL_TEXTURE_2D, data->handle);

	GLTexFormat format = getTextureFormat(image.getFormat());

	#ifndef N_NO_TEX_STORAGE
	gl::glTexStorage2D(GL_TEXTURE_2D, 1, format.internalFormat, size.x(), size.y());
	if(image.data()) {
		gl::glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x(), size.y(), format.format, format.type, image.data()); // crashes if data = 0...
	}
	#else
	gl::glTexImage2D(GL_TEXTURE_2D, 0, format.internalFormat, size.x(), size.y(), 0, format.format, format.type, image.data());
	#endif

	gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void Texture::prepare(bool sync) const {
	if(!image.isNull()) {
		if(data->lock.trylock()) {
			if(sync) {
				upload();
			} else {
				GLContext::getContext()->addGLTask([=]() {
					upload();
					internal::TextureBinding::dirty();
				});
			}
		}
	} else {
		if(sync) {
			gl::glBindTexture(GL_TEXTURE_2D, data->handle);
		}
	}
}

gl::GLuint Texture::getHandle() const {
	return data->handle;
}

}
}
