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

#ifndef N_GRAPHICS_FRAMEBUFFERPOOL_H
#define N_GRAPHICS_FRAMEBUFFERPOOL_H

#include "FrameBuffer.h"

namespace n {
namespace graphics {

class FrameBufferPool : core::NonCopyable
{
	template<typename T, typename... Args>
	static void filter(uint index, core::Array<FrameBuffer *> &arr, T t, Args... args) {
		filter(index, arr, t);
		filter(index + 1, arr, args...);
	}

	static void filter(uint, core::Array<FrameBuffer *> &) {
	}

	static void filter(uint index, core::Array<FrameBuffer *> &arr, bool att) {
		arr.filter([index, att](FrameBuffer *fb) { return fb->isAttachmentEnabled(index) == att; });
	}

	static void filter(uint index, core::Array<FrameBuffer *> &arr, ImageFormat::Format format) {
		arr.filter([index, format](FrameBuffer *fb) { return fb->isAttachmentEnabled(index) && fb->getAttachement(index).getFormat() == format; });
	}

	public:
		FrameBufferPool() {
		}

		~FrameBufferPool() {
		}

		template<typename... Args>
		FrameBuffer *get(const math::Vec2ui &size, bool depth, Args... args) {
			core::Array<FrameBuffer *> arr = buffers.filtered([depth, size](FrameBuffer *fb) { return fb->getSize() == size && fb->isDepthEnabled() == depth; });
			filter(0, arr, args...);
			if(arr.isEmpty()) {
				return new FrameBuffer(size, depth, args...);
			}
			buffers.remove(arr.first());
			return arr.first();
		}

		void add(FrameBuffer *fb) {
			if(fb) {
				buffers.append(fb);
			}
		}

	private:
		core::Array<FrameBuffer *> buffers;
};

}
}

#endif // N_GRAPHICS_FRAMEBUFFERPOOL_H
