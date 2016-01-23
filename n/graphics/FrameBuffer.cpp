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
	const FrameBufferBase *fb = GLContext::getContext()->getFrameBuffer();
	bind();
	uint att = getMaxAttachment();
	for(uint i = 0; i != att; i++) {
		if(isAttachmentEnabled(i)) {
			if(!attachments[i].synchronize(true)) {
				fatal("Unable to create Attachment.");
			}
			gl::bindTexture(Texture2D, attachments[i].getHandle());
			gl::attachFramebufferTexture(i, Texture2D, attachments[i].getHandle());
		} else {
			gl::attachFramebufferTexture(i, Texture2D, 0);
		}
	}
	if(depth) {
		if(!depth->synchronize(true)) {
			fatal("Unable to create depth Attachment.");
		}
		gl::bindTexture(Texture2D, depth->getHandle());
		gl::attachFramebufferTexture(gl::DepthAtt, Texture2D,  depth->getHandle());
	} else {
		gl::attachFramebufferTexture(gl::DepthAtt, Texture2D,  0);
	}
	gl::drawBuffers(att, drawBuffers);
	internal::TextureBinding::dirty();

	gl::assertFboStatus();

	if(fb) {
		fb->bind();
	} else {
		FrameBuffer::unbind();
	}
}

void FrameBuffer::clear(bool color, bool depth) {
	gl::BitField bits = (color ? gl::ColorBit : 0) | (depth ? gl::DepthBit : 0);
	MaterialRenderData().bind(RenderFlag::DepthWriteOnly);
	gl::clear(bits);
}

void FrameBuffer::blit(uint slot, bool depth) const {
	if(GLContext::getContext()->getFrameBuffer() != this) {
		gl::bindFramebuffer(gl::ReadBuffer, handle);
	}
	depth |= slot == Depth;
	bool color = slot != Depth;
	if(color) {
		gl::readBuffer(gl::Attachment(gl::ColorAtt0 + slot));
	}
	gl::BitField bits = (color ? gl::ColorBit : 0) | (depth ? gl::DepthBit : 0);
	MaterialRenderData().bind(RenderFlag::DepthWriteOnly);
	#warning Blitting into framebuffer 0 with a float texture will cause an error
	gl::blitFramebuffer(0, 0, getSize().x(), getSize().y(), 0, 0, GLContext::getContext()->getViewport().x(), GLContext::getContext()->getViewport().y(), bits, gl::Nearest);
}

}
}
