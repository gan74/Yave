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

#ifndef N_GRAPHICS_BUFFERRENDERER
#define N_GRAPHICS_BUFFERRENDERER

#include "BufferableRenderer.h"
#include "BufferedRenderer.h"

namespace n {
namespace graphics {

class BufferRenderer : public BufferedRenderer
{
	public:
		BufferRenderer(BufferableRenderer *c, const math::Vec2ui &s = math::Vec2ui()) : BufferedRenderer(s), child(c) {
		}

		virtual ~BufferRenderer() {
		}

		virtual void *prepare() override {
			return child->prepare();
		}

		virtual void render(void *ptr) override {
			getFrameBuffer().bind();
			child->render(ptr);
		}


	private:
		BufferableRenderer *child;
};

}
}


#endif // N_GRAPHICS_TBUFFERRENDERER


