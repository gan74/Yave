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
	BindingData() : handle(0), type(TextureType::Texture2D), sampler(0) {
	}

	gl::GLuint handle;
	gl::GLenum type;
	gl::GLuint sampler;
};

static BindingData *bindings = 0;
static uint active = 0;
static gl::GLuint samplers[TextureSampler::Default][2] = {0};

void init() {
	gl::glGenSamplers(uint(TextureSampler::Default) * 2, samplers[0]);
	gl::GLenum glSamplers[][2] = {{GL_NEAREST, GL_NEAREST_MIPMAP_NEAREST}, {GL_LINEAR, GL_LINEAR_MIPMAP_NEAREST}, {GL_LINEAR, GL_LINEAR_MIPMAP_LINEAR}};
	for(uint b = 0; b != 2; b++) {
		for(uint i = 0; i != TextureSampler::Default; i++) {
			gl::GLuint smp = samplers[i][b];
			gl::glBindSampler(active, smp);
			gl::glSamplerParameteri(smp, GL_TEXTURE_WRAP_T, GL_REPEAT);
			gl::glSamplerParameteri(smp, GL_TEXTURE_WRAP_S, GL_REPEAT);
			gl::glSamplerParameteri(smp, GL_TEXTURE_MAG_FILTER, glSamplers[i][0]);
			gl::glSamplerParameteri(smp, GL_TEXTURE_MIN_FILTER, glSamplers[i][b]);
		}
	}
	bindings = new BindingData[maxTextures()];
}

void setActive(uint slot) {
	if(slot != active) {
		gl::glActiveTexture(GL_TEXTURE0 + slot);
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
		t.sampler = samplers[smp][tex->hasMips];
	}
	if(bindings[slot].handle != t.handle) {
		setActive(slot);
		gl::glBindTexture(bindings[slot].type = t.type, bindings[slot].handle = t.handle);
	}
	if(bindings[slot].sampler != t.sampler) {
		gl::glBindSampler(slot, bindings[slot].sampler = t.sampler);
	}
}

void TextureBinding::dirty() {
	if(bindings) {
		gl::glBindTexture(bindings[active].type, bindings[active].handle);
		gl::glBindSampler(active, bindings[active].sampler);
	}
}

void TextureBinding::clear() {
	if(bindings) {
		for(uint i = 0; i != maxTextures(); i++) {
			gl::glActiveTexture(GL_TEXTURE0 + (active = i));
			gl::glBindTexture(bindings[active].type, bindings[active].handle = 0);
			gl::glBindSampler(active, bindings[active].sampler = 0);
		}
	}
}

}
}
}
