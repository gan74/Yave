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

#include "UniformBuffer.h"
#include "GLContext.h"
#include "VertexArrayFactory.h"

namespace n {
namespace graphics {

namespace internal {
uint maxUboBytes() {
	return GLContext::getContext()->getHWInt(GLContext::MaxUBOBytes);
}
}

DynamicBufferBase::Data::Data(uint si, BufferTarget tpe) : type(tpe), size(si), buffer(malloc(size)), handle(0), modified(true) {
}

DynamicBufferBase::Data::~Data() {
	free(buffer);
	if(handle) {
		gl::Handle h = handle;
		GLContext::getContext()->addGLTask([=]() {
			gl::deleteBuffer(h);
		});
	}
}

void DynamicBufferBase::Data::update(bool forceBind) const {
	if(modified) {
		if(!forceBind) {
			gl::bindVertexArray(VertexArrayBase::currentVao() = 0);
		}
		if(!handle) {
			handle = gl::createBuffer();
			gl::bindBuffer(type, handle);
			gl::bufferData(type, size, buffer, gl::Dynamic);
		} else {
			gl::bindBuffer(type, handle);
			gl::bufferSubData(type, 0, size, 0);
			gl::bufferSubData(type, 0, size, buffer);
		}
		modified = false;
	} else if(forceBind) {
		gl::bindBuffer(type, handle);
	}
}

DynamicBufferBase::DynamicBufferBase(uint si, BufferTarget tpe) : data(new Data(si, tpe)) {
}

void DynamicBufferBase::update(bool force) {
	data->update(force);
}

DynamicBufferBase::DynamicBufferBase() : data(0) {
}

}
}
