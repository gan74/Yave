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

#include <n/utils.h>
#include "Texture.h"
#include "GL.h"

namespace n {
namespace graphics {

class FrameBuffer : core::NonCopyable
{
	public:
		static constexpr uint Depth = -1u;
		FrameBuffer(const math::Vec2ui &s);
		~FrameBuffer();


		void setAttachmentFormat(uint slot, ImageFormat format);
		void setAttachmentEnabled(uint slot, bool enabled);
		void setDepthEnabled(bool enabled);
		bool isAttachmentEnabled(uint slot) const;
		bool isDepthEnabled() const;

		bool isActive() const;

		void bind() const;
		static void unbind();

		void blit(bool color = true, bool dept = true) const;

		math::Vec2ui getSize() const {
			return base;
		}

		Texture getAttachement(uint slot) const {
			return slot == Depth ? getDepthAttachement() : attachments[slot];
		}

		Texture getDepthAttachement() const {
			return depth ? *depth : Texture();
		}


	private:
		void setModified();
		void setUnmodified() const;

		math::Vec2ui base;
		Texture *attachments;
		Texture *depth;
		gl::GLenum *drawBuffers;
		gl::GLuint handle;

		mutable bool modified;
};

}
}

#endif // N_GRAPHICS_FRAMEBUFFER_H
