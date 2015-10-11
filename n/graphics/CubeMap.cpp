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

/*namespace n {
namespace graphics {

CubeMap::CubeMap(const Texture &top, const Texture &bottom, const Texture &right, const Texture &left, const Texture &front, const Texture &back) : TextureBase<TextureCube>(), sides{top, bottom, right, left, front, back} {
}

void CubeMap::upload() const {
	 gl::GLenum glSides[] = {
		GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Z,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		GL_TEXTURE_CUBE_MAP_POSITIVE_X,
		GL_TEXTURE_CUBE_MAP_NEGATIVE_X
	};
	gl::glGenTextures(1, &(data->handle));
	gl::glBindTexture(GL_TEXTURE_CUBE_MAP, data->handle);
	gl::GLubyte* data = new gl::GLubyte[sides[0].getSize().mul() * 4];
	for(uint i = 0; i != 6; i++) {
		gl::glBindTexture(GL_TEXTURE_2D, sides[i].getHandle());
		math::Vec2ui size = sides[i].getSize();
		gl::glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
		gl::glTexImage2D(glSides[i], 0, GL_RGBA8, size.x(), size.y(), 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	gl::glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	gl::glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	gl::glGenerateMipmap(GL_TEXTURE_CUBE_MAP);
	delete[] data;
}

void CubeMap::prepare(bool sync) const {
	if(isComplete()) {
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
		for(uint i = 0; i != 6; i++) {
			if(sides[i].isNull()) {
				sides[i].prepare(sync);
			}
		}
		if(sync) {
			gl::glBindTexture(GL_TEXTURE_CUBE_MAP, data->handle);
		}
	}
}

}
}*/
