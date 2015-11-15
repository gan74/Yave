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
#include "TextureBinding.h"
#include "GLContext.h"
#include "StaticBuffer.h"
#include "GL.h"

#define N_NO_TEX_STORAGE

namespace n {
namespace graphics {



bool Texture::isHWSupported(ImageFormat format) {
	return gl::isHWSupported(format);
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
	data->handle = gl::createTexture();
	gl::bindTexture(Texture2D, data->handle);

	gl::TextureFormat format = gl::getTextureFormat(image.getFormat());

	data->hasMips &= isMipCapable();
	#ifndef N_NO_TEX_STORAGE
	uint maxMips = getMipmapLevels();
	gl::glTexStorage2D(GL_TEXTURE_2D,  maxMips, format.internalFormat, size.x(), size.y());
	if(image.data()) {
		gl::glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, size.x(), size.y(), format.format, format.type, image.data()); // crashes if data = 0...
	}
	#else
	gl::texImage2D(Texture2D, 0, size.x(), size.y(), 0, format, image.data());
	#endif

	if(hasMipmaps()) {
		gl::generateMipmap(Texture2D);
	}

	if(GLContext::getContext()->getHWInt(GLContext::BindlessTextureSupport)) {
		data->bindless = gl::getTextureSamplerHandle(data->handle, GLContext::getContext()->getDefaultSampler(), hasMipmaps());
		gl::makeTextureHandleResident(data->bindless);
	}
}

bool Texture::prepare(bool sync) const {
	if(getHandle()) {
		return true;
	}
	if(image.isLoading()) {
		return false;
	}
	if(image.isNull()) {
		return true;
	}
	if(data->lock.trylock()) {
		if(sync) {
			upload();
		} else {
			Texture self(*this);
			GLContext::getContext()->addGLTask([=]() {
				self.upload();
			});
		}
	}
	return getHandle();
}

bool Texture::synchronize(bool immediate) {
	return prepare(immediate);
}

}
}
