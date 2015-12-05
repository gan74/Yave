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

#include "TextureArray.h"
#include "TextureBinding.h"

namespace n {
namespace graphics {

TextureArray::TextureArray(const math::Vec3ui &si, ImageFormat format) : TextureBase<Texture2DArray>(), size(si), imgFormat(format), textures(0) {
}

TextureArray::TextureArray() : TextureArray(math::Vec3ui(0), ImageFormat(ImageFormat::RGBA8)) {
}

TextureArray::~TextureArray() {
	delete[] textures;
}

const math::Vec3ui &TextureArray::getSize() const {
	return size;
}

Texture TextureArray::getTexture(uint index) const {
	if(isNull() && !prepare(true)) {
		return Texture();
	}
	if(!textures) {
		textures = new Texture[size.z()];
	}

	if(textures[index].isNull()) {
		internal::TextureBase base(Texture2D);
		base.data->handle = gl::createTexture2DView(data->handle, index, 1, gl::getTextureFormat(imgFormat));
		base.data->hasMips = false;
		base.data->lock.lock();
		base.data->parent = data->handle;
		textures[index] = Texture(base, Image(new ImageData(size.sub(2), imgFormat)));
	}
	return textures[index];
}

void TextureArray::upload() const {
	if(!size.mul()) {
		fatal("Invalid image size.");
	}
	data->hasMips &= isMipCapable();
	gl::TextureFormat format = gl::getTextureFormat(imgFormat);

	data->handle = gl::createTexture2DArray(size, 1, format, 0);

	if(GLContext::getContext()->getHWInt(GLContext::BindlessTextureSupport)) {
		data->bindless = gl::getTextureSamplerHandle(data->handle, GLContext::getContext()->getDefaultSampler(), hasMipmaps());
		gl::makeTextureHandleResident(data->bindless);
	}

}

bool TextureArray::synchronize(bool immediate) {
	if(!isNull()) {
		return true;
	}
	bool p = prepare(immediate);
	if(immediate) {
		internal::TextureBinding::dirty();
	}
	return p;
}

bool TextureArray::prepare(bool sync) const {
	if(getHandle()) {
		return true;
	}
	if(data->lock.trylock()) {
		if(sync) {
			upload();
		} else {
			TextureArray self(*this);
			GLContext::getContext()->addGLTask([=]() {
				self.upload();
			});
		}
	}
	return getHandle();
}

}
}
