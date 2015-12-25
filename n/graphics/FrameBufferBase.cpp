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

#include "FrameBufferBase.h"
#include "GLContext.h"

namespace n {
namespace graphics {

FrameBufferBase::FrameBufferBase(const math::Vec2ui &si) : size(si), handle(gl::createFramebuffer()), drawBuffers(new gl::Attachment[getMaxAttachment()]) {
}

FrameBufferBase::~FrameBufferBase() {
	if(isActive()) {
		unbind();
	}
}

uint FrameBufferBase::getMaxAttachment() {
	return GLContext::getContext()->getHWInt(GLContext::MaxFboAttachements);
}

bool FrameBufferBase::isActive() const {
	return GLContext::getContext()->frameBuffer == this;
}

void FrameBufferBase::bind() const {
	if(GLContext::getContext()->frameBuffer != this) {
		gl::bindFramebuffer(gl::FrameBuffer, handle);
		math::Vec2ui vp = GLContext::getContext()->getViewport();
		GLContext::getContext()->frameBuffer = this;
		if(vp != size) {
			gl::setViewport(math::Vec2i(0), size);
		}
	}
}

void FrameBufferBase::unbind() {
	if(GLContext::getContext()->frameBuffer) {
		gl::bindFramebuffer(gl::FrameBuffer, 0);
		const FrameBufferBase *fb = GLContext::getContext()->frameBuffer;
		GLContext::getContext()->frameBuffer = 0;
		if(GLContext::getContext()->getViewport() != fb->getSize()) {
			gl::setViewport(math::Vec2i(0), GLContext::getContext()->getViewport());
		}
	}
}

}
}

