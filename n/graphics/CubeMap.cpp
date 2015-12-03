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

namespace n {
namespace graphics {

CubeMap::CubeMap(const Texture &pz, const Texture &nz, const Texture &py, const Texture &ny, const Texture &px, const Texture &nx) : TextureBase<TextureCube>(), sides{pz, nz, py, ny, px, nx} {
}

void CubeMap::upload() const {
	data->handle = gl::createTexture();
	//gl::bindTexture(TextureCube, data->handle);


	gl::texCube3D(data->handle, gl::getTextureFormat(sides[0].getFormat()), sides[0].getSize(), sides[0].getHandle(), sides[1].getHandle(), sides[2].getHandle(), sides[3].getHandle(), sides[4].getHandle(), sides[5].getHandle());


}

bool CubeMap::prepare(bool sync) const {
	if(getHandle()) {
		return true;
	}
	if(prepareSides(sync)) {
		if(data->lock.trylock()) {
			if(sync) {
				upload();
			} else {
				CubeMap self(*this);
				GLContext::getContext()->addGLTask([=]() {
					self.upload();
				});
			}
		}
	}
	return getHandle();
}


bool CubeMap::prepareSides(bool sync) const {
	bool rdy = true;
	for(uint i = 0; i != 6; i++) {
		rdy &= sides[i].prepare(sync);
	}
	return rdy;
}

bool CubeMap::synchronize(bool immediate) {
	if(!isNull()) {
		return true;
	}
	bool p = prepare(immediate);
	if(immediate) {
		internal::TextureBinding::dirty();
	}
	return p;
}


}
}
