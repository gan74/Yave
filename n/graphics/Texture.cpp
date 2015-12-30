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

Texture::Texture(const internal::TextureBase &base, Image im) : TextureBase<Texture2D>(base), image(im) {
	if(data->handle) {
		if(!data->bindless && GLContext::getContext()->getHWInt(GLContext::BindlessTextureSupport)) {
			data->bindless = gl::getTextureBindlessHandle(data->handle, TextureSampler::Trilinear, hasMipmaps());
			gl::makeTextureHandleResident(data->bindless);
		}
	}
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
	if(!getHandle()) {
		math::Vec2ui size = image.getSize();
		if(!size.mul()) {
			fatal("Invalid image size.");
		}
		data->hasMips &= isMipCapable();
		gl::TextureFormat format = gl::getTextureFormat(image.getFormat());

		data->handle = gl::createTexture2D(size, getMipmapLevel(), format, image.data());

		if(GLContext::getContext()->getHWInt(GLContext::BindlessTextureSupport)) {
			data->bindless = gl::getTextureBindlessHandle(data->handle, TextureSampler::Trilinear, hasMipmaps());

			gl::makeTextureHandleResident(data->bindless);
		}

		internal::TextureBinding::dirty();
	}
}

bool Texture::synchronize(bool sync) const {
	if(getHandle()) {
		return true;
	}
	if(image.isLoading()) {
		return false;
	}
	if(image.isNull()) {
		return true;
	}
	if(sync) {
		data->lock.trylock();
		upload();
	} else {
		if(data->lock.trylock()) {
			Texture self(*this);
			GLContext::getContext()->addGLTask([=]() {
				self.upload();
			});
		}
	}
	return getHandle();
}


}
}
