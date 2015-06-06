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

#ifndef N_GRAPHICS_FRAMEBUFFERRENDERER
#define N_GRAPHICS_FRAMEBUFFERRENDERER

#include <n/utils.h>
#include "BufferableRenderer.h"

namespace n {
namespace graphics {

class FrameBufferRenderer : public BufferableRenderer
{
	public:
		FrameBufferRenderer(BufferedRenderer *c, uint s = 0) : BufferableRenderer(), child(c), slot(s) {
		}

	protected:
		virtual void *prepare() override {
			return child->prepare();
		}

		virtual void render(void *ptr) override {
			child->render(ptr);
			if(GLContext::getContext()->getFrameBuffer() == &child->getFrameBuffer()) {
				FrameBuffer::unbind();
			}
			child->getFrameBuffer().blit(slot);
		}

	private:
		BufferedRenderer *child;
		uint slot;


};

}
}

#endif // N_GRAPHICS_FRAMEBUFFERRENDERER

