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

#ifndef N_GRAPHICS_FRAMEBUFFER_H
#define N_GRAPHICS_FRAMEBUFFER_H

#include "FrameBufferBase.h"
#include "Texture.h"

namespace n {
namespace graphics {

class FrameBuffer : public FrameBufferBase
{
	void assertAttachments(uint index) {
		if(index >= getMaxAttachment()) {
			fatal("Too many framebuffer attachments.");
		}
	}

	template<typename T, typename U>
	void setupAttachments(uint index, T t, U u) {
		setupAttachment(index, t);
		setupAttachment(index + 1, u);
	}

	template<typename T, typename... Args>
	void setupAttachments(uint index, T t, Args... args) {
		setupAttachment(index, t);
		setupAttachments(index + 1, args...);
	}

	void setupAttachments(uint index) {
		assertAttachments(index);
	}

	void setupAttachment(uint index, bool enabled) {
		assertAttachments(index);
		drawBuffers[index] = enabled ? gl::Attachment(gl::ColorAtt0 + index) : gl::NoAtt;
	}

	void setupAttachment(uint index, ImageFormat::Format format) {
		assertAttachments(index);
		drawBuffers[index] = gl::Attachment(gl::ColorAtt0 + index);
		attachments[index] = Image(getSize(), format);
	}

	public:
		static constexpr uint Depth = gl::DepthAtt;

		~FrameBuffer();

		bool isAttachmentEnabled(uint slot) const;
		bool isDepthEnabled() const;

		static void clear(bool color, bool depth);

		void blit(uint slot = 0, bool depth = false) const;

		Texture getAttachment(uint slot) const {
			return slot == Depth ? getDepthAttachment() : attachments[slot];
		}

		Texture getDepthAttachment() const {
			return depth ? *depth : Texture();
		}

		template<typename... Args>
		FrameBuffer(const math::Vec2ui &s, bool depthEnabled, Args... args) : FrameBufferBase(s), attachments(new Texture[getMaxAttachment()]) {
			Image baseImage(size);
			for(uint i = 0; i != getMaxAttachment(); i++) {
				drawBuffers[i] = gl::NoAtt;
				attachments[i] = Texture(baseImage);
			}
			depth = depthEnabled ? new Texture(Image(size, ImageFormat::Depth32)) : 0;
			setupAttachments(0, args...);
			setup();
		}

	private:
		friend class FrameBufferPool;

		void setup();

		Texture *attachments;
		Texture *depth;
};

}
}

#endif // N_GRAPHICS_FRAMEBUFFER_H
