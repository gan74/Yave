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

namespace n {
namespace graphics {
namespace internal {

uint maxTextures() {
	return GLContext::getContext()->getHWInt(GLContext::MaxTextures);
}

struct BindingData
{
	BindingData() : handle(0), type(TextureType::Texture2D) {
	}

	gl::GLuint handle;
	gl::GLenum type;
};

static BindingData *bindings = 0;
static uint active = 0;

TextureBinding::TextureBinding() {
}

void TextureBinding::bind(uint slot) const {
	if(!bindings) {
		bindings = new BindingData[maxTextures()];
	}
	BindingData t;
	if(tex) {
		t.handle = tex->handle;
		t.type = tex->type;
	}
	if(bindings[slot].handle != t.handle) {
		if(slot != active) {
			gl::glActiveTexture(GL_TEXTURE0 + slot);
			active = slot;
		}
		gl::glBindTexture(bindings[slot].type = t.type, bindings[slot].handle = t.handle);
	}
}

void TextureBinding::dirty() {
	if(bindings) {
		gl::glBindTexture(bindings[active].type, bindings[active].handle);
	}
}

}
}
}
