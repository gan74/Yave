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
	BindingData() : handle(0), type(TextureType::Texture2D), sampler(TextureSampler::Default) {
	}

	gl::Handle handle;
	TextureType type;
	TextureSampler sampler;
};

static BindingData *bindings = 0;
static uint active = 0;

void init() {
	bindings = new BindingData[maxTextures()];
}

void setActive(uint slot) {
	if(slot != active) {
		gl::activeTexture(slot);
		active = slot;
	}
}

TextureBinding::TextureBinding() : sampler(TextureSampler::Default) {
}

void TextureBinding::bind(uint slot) const {
	TextureSampler smp = sampler == TextureSampler::Default ? GLContext::getContext()->getDefaultSampler() : sampler;
	if(!bindings) {
		init();
	}
	BindingData t;
	if(tex) {
		t.handle = tex->handle;
		t.type = tex->type;
		t.sampler = smp;
	}
	if(bindings[slot].handle != t.handle) {
		setActive(slot);
		gl::bindTexture(bindings[slot].type = t.type, bindings[slot].handle = t.handle);
	}
	if(bindings[slot].sampler != t.sampler) {
		gl::bindSampler(slot, bindings[slot].sampler = t.sampler);
	}
}

void TextureBinding::dirty() {
	if(bindings) {
		gl::bindTexture(bindings[active].type, bindings[active].handle);
		gl::bindSampler(active, bindings[active].sampler);
	}
}

}
}
}
