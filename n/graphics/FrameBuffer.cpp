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

namespace n {
namespace graphics {

uint getMaxAttachment() {
	return GLContext::getContext()->getHWInt(GLContext::MaxFboAttachements);
}

FrameBuffer::FrameBuffer(const math::Vec2ui &s) : base(s), attachments(new Texture[getMaxAttachment()]), depth(0), drawBuffers(new gl::GLenum[getMaxAttachment()]), handle(0), modified(false) {
	Image baseImage(base);
	for(uint i = 0; i != getMaxAttachment(); i++) {
		drawBuffers[i] = GL_NONE;
		attachments[i] = Texture(baseImage);
	}
	gl::glGenFramebuffers(1, &handle);
	setAttachmentEnabled(0, true);
	setDepthEnabled(true);
}

FrameBuffer::~FrameBuffer() {
	if(isActive()) {
		unbind();
	}
	gl::glDeleteFramebuffers(1, &handle);
	delete depth;
	delete[] drawBuffers;
	delete[] attachments;
}

void FrameBuffer::setAttachmentEnabled(uint slot, bool enabled) {
	if(slot == Depth) {
		setDepthEnabled(enabled);
		return;
	}
	if(!enabled && isAttachmentEnabled(slot)) {
		drawBuffers[slot] = GL_NONE;
		attachments[slot] = Texture(Image());
		setModified();
	}
	if(enabled && !isAttachmentEnabled(slot)) {
		drawBuffers[slot] = GL_COLOR_ATTACHMENT0 + slot;
		setModified();
	}
}

void FrameBuffer::setAttachmentFormat(uint slot, ImageFormat format) {
	if(slot == Depth) {
		if(format != ImageFormat::Depth32) {
			fatal("Invalid depth format.");
		}
	}
	if(attachments[slot].getFormat() != format) {
		attachments[slot] = Image(getSize(), format);
		setModified();
	}
}

void FrameBuffer::setDepthEnabled(bool enabled) {
	if(enabled == isDepthEnabled()) {
		return;
	}
	if(enabled) {
		depth = new Texture(Image(base, ImageFormat::Depth32));
	} else {
		delete depth;
		depth = 0;
	}
	setModified();
}

bool FrameBuffer::isDepthEnabled() const {
	return depth;
}

bool FrameBuffer::isAttachmentEnabled(uint slot) const {
	return slot == Depth ? isDepthEnabled() : drawBuffers[slot] != GL_NONE;
}

void FrameBuffer::setModified() {
	modified = true;
	setUnmodified();
}

void FrameBuffer::setUnmodified() const {
	if(!modified || !isActive()) {
		return;
	}
	for(uint i = 0; i != getMaxAttachment(); i++) {
		if(isAttachmentEnabled(i)) {
			attachments[i].prepare(true);
			gl::glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, attachments[i].getHandle(), 0);
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

	gl::glDrawBuffers(getMaxAttachment(), drawBuffers);
	internal::TextureBinding::dirty();
	modified = false;

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
		setUnmodified();
	}
}

void FrameBuffer::unbind() {
	if(GLContext::getContext()->frameBuffer) {
		gl::glBindFramebuffer(GL_FRAMEBUFFER, 0);
		if(GLContext::getContext()->getViewport() != GLContext::getContext()->getFrameBuffer()->getSize()) {
			gl::glViewport(0, 0, GLContext::getContext()->getViewport().x(), GLContext::getContext()->getViewport().y());
		}
		GLContext::getContext()->frameBuffer = 0;
	}
}

void FrameBuffer::blit(bool color, bool dept) const {
	if(GLContext::getContext()->frameBuffer != this) {
		gl::glBindFramebuffer(GL_READ_FRAMEBUFFER, handle);
	}
	gl::glBlitFramebuffer(0, 0, getSize().x(), getSize().y(), 0, 0, GLContext::getContext()->getViewport().x(), GLContext::getContext()->getViewport().y(), (color ? GL_COLOR_BUFFER_BIT : 0) | (dept ? GL_DEPTH_BUFFER_BIT : 0), GL_NEAREST);
	//gl::glBindFramebuffer(GL_READ_FRAMEBUFFER, GLContext::getContext()->frameBuffer ? GLContext::getContext()->frameBuffer->handle : 0);
}

}
}
