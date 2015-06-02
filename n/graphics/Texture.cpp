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

//#define N_NO_TEX_STORAGE

namespace n {
namespace graphics {

GLTexFormat getTextureFormat(ImageFormat format) {
	switch(format) {
		case ImageFormat::Depth32:
			return GLTexFormat(GL_DEPTH_COMPONENT, GL_DEPTH_COMPONENT32, GL_FLOAT);
		break;
		case ImageFormat::F32:
			return GLTexFormat(GL_RED, GL_R32F, GL_FLOAT);
		break;
		case ImageFormat::RGBA32F:
			return GLTexFormat(GL_RGBA, GL_RGBA32F, GL_FLOAT);
		break;
		case ImageFormat::RG16:
			return GLTexFormat(GL_RG, GL_RG16, GL_UNSIGNED_SHORT);
		break;
		case ImageFormat::RGBA8:
			return GLTexFormat(GL_RGBA, GL_RGBA8, GL_UNSIGNED_BYTE);
		break;
		case ImageFormat::RGB8:
			return GLTexFormat(GL_RGB, GL_RGB8, GL_UNSIGNED_BYTE);
		break;
		case ImageFormat::RGBA16:
			return GLTexFormat(GL_RGBA, GL_RGBA16, GL_UNSIGNED_SHORT);
		break;
		default:
			return fatal("Unsuported texture format.");
		break;
	}
}

bool Texture::isHWSupported(ImageFormat format) {
	if(format == ImageFormat::RGB10A2) {
		return false;
	}
	static core::Map<ImageFormat, bool> support;
	core::Map<ImageFormat, bool>::const_iterator it = support.find(format);
	if(it == support.end()) {
		gl::GLint s = 0;
		gl::glGetInternalformativ(GL_TEXTURE_2D, getTextureFormat(format).internalFormat, GL_INTERNALFORMAT_SUPPORTED, sizeof(gl::GLint), &s);
		/*gl::GLint p = 0;
		gl::glGetInternalformativ(GL_TEXTURE_2D, getTextureFormat(format).internalFormat, GL_INTERNALFORMAT_PREFERRED, sizeof(gl::GLint), &p);
		if(p != gl::GLint(getTextureFormat(format).internalFormat)) {
			std::cerr<<"Texture format not fully supported"<<std::endl;
		}*/
		return support[format] = (s == GL_TRUE);
	}
	return (*it)._2;
}

Texture::Texture(const Image &i, bool mip) : TextureBase<Texture2D>(), image(i) {
	data->hasMips = mip;
}

Texture::Texture() : TextureBase<Texture2D>() {
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
	if(getHandle()) {
		gl::glDeleteTextures(1, &(data->handle));
	}
	gl::glGenTextures(1, &(data->handle));
	gl::glBindTexture(GL_TEXTURE_2D, data->handle);

	GLTexFormat format = getTextureFormat(image.getFormat());

	data->hasMips &= isMipCapable();
	uint maxMips = hasMipmaps() ? 1 + floor(log2(getSize().max())) : 1;
	#ifndef N_NO_TEX_STORAGE
	gl::glTexStorage2D(GL_TEXTURE_2D,  maxMips, format.internalFormat, size.x(), size.y());
	if(image.data()) {
		gl::glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x(), size.y(), format.format, format.type, image.data()); // crashes if data = 0...
	}
	#else
	gl::glTexImage2D(GL_TEXTURE_2D, 0, format.internalFormat, size.x(), size.y(), 0, format.format, format.type, image.data());
	#endif
	if(hasMipmaps()) {
		gl::glGenerateMipmap(GL_TEXTURE_2D);
	}

	gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl::glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, hasMipmaps() ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
}

void Texture::computeMipMaps(bool sync) {
	if(hasMipmaps() || !isMipCapable()) {
		return;
	}
	data->hasMips = true;
	if(getHandle()) {
		if(sync) {
			upload();
		} else {
			GLContext::getContext()->addGLTask([=]() {
				upload();
				internal::TextureBinding::dirty();
			});
		}
	}
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

}
}
