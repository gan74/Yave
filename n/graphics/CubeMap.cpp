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

CubeMap::CubeMap(const TextureArray &array) : texArray(array) {
}

void CubeMap::upload() const {
	if(!getHandle()) {
		if(texArray.getSize().z() < 6) {
			logMsg("Cube map build from array with less than 6 elements", ErrorLog);
			return;
		}
		data->handle = gl::createTextureCubeView(texArray.getHandle(), 1, gl::getTextureFormat(texArray.getFormat()));
		data->parent = texArray.getHandle();

		if(GLContext::getContext()->getHWInt(GLContext::BindlessTextureSupport)) {
			data->bindless = gl::getTextureSamplerHandle(data->handle, GLContext::getContext()->getDefaultSampler(), hasMipmaps());
			gl::makeTextureHandleResident(data->bindless);
		}

		internal::TextureBinding::dirty();
	}
}

bool CubeMap::synchronize(bool sync) const {
	if(getHandle()) {
		return true;
	}
	if(!texArray.synchronize(sync)) {
		return false;
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

}
}
