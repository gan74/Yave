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

#include "TextureBinding.h"
#include <iostream>

namespace n {
namespace graphics {
namespace internal {

uint maxTextures() {
	return GLContext::getContext()->getHWInt(GLContext::MaxTextures);
}

gl::GLuint *TextureBinding::bindings = 0;
uint TextureBinding::active = 0;

TextureBinding::TextureBinding() {
}

TextureBinding &TextureBinding::operator=(const Texture &t) {
	tex = t;
	return *this;
}

void TextureBinding::bind(uint slot) const {
	if(!bindings) {
		bindings = new gl::GLuint[maxTextures()];
		for(uint i = 0; i != maxTextures(); i++) {
			bindings[i] = 0;
		}
	}
	if(bindings[slot] != tex.getHandle()) {
		if(slot != active) {
			gl::glActiveTexture(GL_TEXTURE0 + slot);
			active = slot;
		}
		gl::glBindTexture(GL_TEXTURE_2D, bindings[slot] = tex.getHandle());
	} else {
		tex.prepare();
	}
}

void TextureBinding::dirty() {
	if(bindings) {
		gl::glBindTexture(GL_TEXTURE_2D, bindings[active]);
	}
}

}
}
}
