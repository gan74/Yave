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

#include "FrameBuffer.h"
#include "TextureBinding.h"
#include "Material.h"

namespace n {
namespace graphics {

FrameBuffer::~FrameBuffer() {
	if(isActive()) {
		unbind();
	}
	gl::glDeleteFramebuffers(1, &handle);
	delete depth;
	delete[] drawBuffers;
	delete[] attachments;
}

bool FrameBuffer::isDepthEnabled() const {
	return depth;
}

bool FrameBuffer::isAttachmentEnabled(uint slot) const {
	return slot == Depth ? isDepthEnabled() : drawBuffers[slot] != GL_NONE;
}

void FrameBuffer::setup() {
	const FrameBuffer *fb = GLContext::getContext()->frameBuffer;
	gl::glGenFramebuffers(1, &handle);
	bind();
	uint att = getMaxAttachment();
	for(uint i = 0; i != att; i++) {
		if(isAttachmentEnabled(i)) {
			attachments[i].prepare(true);
			gl::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, attachments[i].getHandle(), 0);
			if(!attachments[i].getHandle()) {
				fatal("Unable to create attachement.");
			}
		} else {
			gl::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, 0, 0);
		}
	}
	if(depth) {
		depth->prepare(true);
		gl::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,  depth->getHandle(), 0);
	} else {
		gl::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,  0, 0);
	}
	gl::glDrawBuffers(att, drawBuffers);
	internal::TextureBinding::dirty();

	if(gl::glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
		switch(gl::glCheckFramebufferStatus(GL_FRAMEBUFFER)) {
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT:
				fatal("Unable to modify frame-buffer-object : missing attachment.");
			break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT:
				fatal("Unable to modify frame-buffer-object : incomplete attachment.");
			break;
			case GL_FRAMEBUFFER_UNSUPPORTED:
				fatal("Unable to modify frame-buffer-object : unsuported feature.");
			break;
			default:
				fatal("Unable to modify frame-buffer-object.");
			break;
		}
	}
	if(fb) {
		fb->bind();
	} else {
		FrameBuffer::unbind();
	}
}

bool FrameBuffer::isActive() const {
	return GLContext::getContext()->frameBuffer == this;
}

void FrameBuffer::bind() const {
	if(GLContext::getContext()->frameBuffer != this) {
		gl::glBindFramebuffer(GL_FRAMEBUFFER, handle);
		math::Vec2ui vp = GLContext::getContext()->getViewport();
		GLContext::getContext()->frameBuffer = this;
		if(vp != base) {
			gl::glViewport(0, 0, base.x(), base.y());
		}
	}
}

void FrameBuffer::unbind() {
	if(GLContext::getContext()->frameBuffer) {
		gl::glBindFramebuffer(GL_FRAMEBUFFER, 0);
		const FrameBuffer *fb = GLContext::getContext()->frameBuffer;
		GLContext::getContext()->frameBuffer = 0;
		if(GLContext::getContext()->getViewport() != fb->getSize()) {
			gl::glViewport(0, 0, GLContext::getContext()->getViewport().x(), GLContext::getContext()->getViewport().y());
		}
	}
}

void FrameBuffer::clear(bool color, bool depth) {
	gl::GLbitfield bits = (color ? GL_COLOR_BUFFER_BIT : 0) | (depth ? GL_DEPTH_BUFFER_BIT : 0);
	Material().bind(RenderFlag::DepthWriteOnly);
	gl::glClear(bits);
}

void FrameBuffer::blit(uint slot, bool depth) const {
	if(GLContext::getContext()->frameBuffer != this) {
		gl::glBindFramebuffer(GL_READ_FRAMEBUFFER, handle);
	}
	depth |= slot == Depth;
	bool color = slot != Depth;
	if(color) {
		static uint readBuffer = 0;
		if(slot != readBuffer) {
			gl::glReadBuffer(GL_COLOR_ATTACHMENT0 + slot);
			readBuffer = slot;
		}
	}
	gl::GLbitfield bits = (color ? GL_COLOR_BUFFER_BIT : 0) | (depth ? GL_DEPTH_BUFFER_BIT : 0);
	Material().bind(RenderFlag::DepthWriteOnly);
	gl::glBlitFramebuffer(0, 0, getSize().x(), getSize().y(), 0, 0, GLContext::getContext()->getViewport().x(), GLContext::getContext()->getViewport().y(), bits, GL_NEAREST);
}

}
}
