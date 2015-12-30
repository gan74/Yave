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

#include "CubeMap.h"
#include "TextureBinding.h"
#include "FrameBuffer.h"
#include "ShaderInstance.h"
#include "Material.h"
#include "VertexArrayObject.h"

namespace n {
namespace graphics {


CubeMap::CubeMap(const Cube &sides) : TextureBase<TextureCube>(), cube(sides) {
}

void CubeMap::upload() const {
	if(!getHandle()) {
		math::Vec2ui size = cube.top.getSize();
		ImageFormat imgF = cube.top.getFormat();

		if(!size.mul() || size.x() != size.y()) {
			fatal("Invalid image size : Cubemaps must be square.");
		}

		const Image *images = reinterpret_cast<const Image *>(&cube);
		for(uint i = 0; i != 6; i++) {
			if(images[i].getFormat() != imgF) {
				fatal("Invalid image format : Cubemap images must be of the same format.");
			}
			if(images[i].getSize() != size) {
				fatal("Invalid image size : Cubemap images must be of the same size.");
			}
		}

		//uint mips = Texture::getMipmapLevelForSize(size);
		#warning CubeMap mipmaping disabled
		uint mips = 1;
		data->hasMips = mips > 1;
		data->hasMips = false;
		gl::TextureFormat format = gl::getTextureFormat(imgF);

		const void *datas[] = {cube.top.data(), cube.bottom.data(), cube.right.data(), cube.left.data(), cube.front.data(), cube.back.data()};
		data->handle = gl::createTextureCube(size, mips, format, datas);

		if(GLContext::getContext()->getHWInt(GLContext::BindlessTextureSupport)) {
			data->bindless = gl::getTextureBindlessHandle(data->handle, TextureSampler::Trilinear, hasMipmaps());
			gl::makeTextureHandleResident(data->bindless);
		}

		internal::TextureBinding::dirty();
	}
}

bool CubeMap::synchronize(bool sync) const {
	if(getHandle()) {
		return true;
	}
	if(cubeLoading()) {
		return false;
	}
	if(cubeNull()) {
		return true;
	}
	if(sync) {
		data->lock.trylock();
		upload();
	} else {
		if(data->lock.trylock()) {
			CubeMap self(*this);
			GLContext::getContext()->addGLTask([=]() {
				self.upload();
			});
		}
	}
	return getHandle();
}

bool CubeMap::cubeNull() const {
	return cube.top.isNull() ||
		   cube.bottom.isNull() ||
		   cube.right.isNull() ||
		   cube.left.isNull() ||
		   cube.front.isNull() ||
		   cube.back.isNull();
}

bool CubeMap::cubeLoading() const {
	return cube.top.isLoading() ||
		   cube.bottom.isLoading() ||
		   cube.right.isLoading() ||
		   cube.left.isLoading() ||
		   cube.front.isLoading() ||
		   cube.back.isLoading();
}

}
}
