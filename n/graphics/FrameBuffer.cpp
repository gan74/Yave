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
	gl::deleteFramebuffer(handle);
	delete depth;
	delete[] drawBuffers;
	delete[] attachments;
}

bool FrameBuffer::isDepthEnabled() const {
	return depth;
}

bool FrameBuffer::isAttachmentEnabled(uint slot) const {
	return slot == Depth ? isDepthEnabled() : drawBuffers[slot] != gl::NoAtt;
}

void FrameBuffer::setup() {
	const FrameBuffer *fb = GLContext::getContext()->frameBuffer;
	handle = gl::createFramebuffer();
	bind();
	uint att = getMaxAttachment();
	for(uint i = 0; i != att; i++) {
		if(isAttachmentEnabled(i)) {
			if(!attachments[i].synchronize(true)) {
				fatal("Unable to create attachement.");
			}
			gl::bindTexture(Texture2D, attachments[i].getHandle());
			gl::framebufferTexture2D(gl::FrameBuffer, gl::Attachment(gl::ColorAtt0 + i), Texture2D, attachments[i].getHandle(), 0);
		} else {
			gl::framebufferTexture2D(gl::FrameBuffer, gl::Attachment(gl::ColorAtt0 + i), Texture2D, 0, 0);
		}
	}
	if(depth) {
		if(!depth->synchronize(true)) {
			fatal("Unable to create depth attachement.");
		}
		gl::bindTexture(Texture2D, depth->getHandle());
		gl::framebufferTexture2D(gl::FrameBuffer, gl::DepthAtt, Texture2D,  depth->getHandle(), 0);
	} else {
		gl::framebufferTexture2D(gl::FrameBuffer, gl::DepthAtt, Texture2D,  0, 0);
	}
	gl::drawBuffers(att, drawBuffers);
	internal::TextureBinding::dirty();

	if(gl::checkFramebufferStatus(gl::FrameBuffer) != gl::FboOk) {
		switch(gl::checkFramebufferStatus(gl::FrameBuffer)) {
			case gl::FboMissingAtt:
				fatal("Unable to modify frame-buffer-object : missing attachment.");
			break;
			case gl::FboIncomplete:
				fatal("Unable to modify frame-buffer-object : incomplete attachment.");
			break;
			case gl::FboUnsupported:
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
		gl::bindFramebuffer(gl::FrameBuffer, handle);
		math::Vec2ui vp = GLContext::getContext()->getViewport();
		GLContext::getContext()->frameBuffer = this;
		if(vp != base) {
			gl::setViewport(math::Vec2i(0), base);
		}
	}
}

void FrameBuffer::unbind() {
	if(GLContext::getContext()->frameBuffer) {
		gl::bindFramebuffer(gl::FrameBuffer, 0);
		const FrameBuffer *fb = GLContext::getContext()->frameBuffer;
		GLContext::getContext()->frameBuffer = 0;
		if(GLContext::getContext()->getViewport() != fb->getSize()) {
			gl::setViewport(math::Vec2i(0), GLContext::getContext()->getViewport());
		}
	}
}

void FrameBuffer::clear(bool color, bool depth) {
	gl::BitField bits = (color ? gl::ColorBit : 0) | (depth ? gl::DepthBit : 0);
	Material().bind(RenderFlag::DepthWriteOnly);
	gl::clear(bits);
}

void FrameBuffer::blit(uint slot, bool depth) const {
	if(GLContext::getContext()->frameBuffer != this) {
		gl::bindFramebuffer(gl::ReadBuffer, handle);
	}
	depth |= slot == Depth;
	bool color = slot != Depth;
	if(color) {
		gl::readBuffer(gl::Attachment(gl::ColorAtt0 + slot));
	}
	gl::BitField bits = (color ? gl::ColorBit : 0) | (depth ? gl::DepthBit : 0);
	Material().bind(RenderFlag::DepthWriteOnly);
	#warning Blitting into framebuffer 0 with a float texture will cause an error
	gl::blitFramebuffer(0, 0, getSize().x(), getSize().y(), 0, 0, GLContext::getContext()->getViewport().x(), GLContext::getContext()->getViewport().y(), bits, gl::Nearest);
}

}
}
