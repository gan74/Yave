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

#ifndef N_GRAPHICS_TONEMAPRENDERER
#define N_GRAPHICS_TONEMAPRENDERER

#include "BufferableRenderer.h"
#include "BufferedRenderer.h"

namespace n {
namespace graphics {

class ToneMapRenderer : public BufferableRenderer
{
	public:
		ToneMapRenderer(BufferedRenderer *c, uint s = 0);
		~ToneMapRenderer();

		virtual void *prepare() override;
		virtual void render(void *ptr) override;


	private:
		BufferedRenderer *child;
		FrameBuffer *buffers[2];
		uint slot;
};

}
}


#endif // N_GRAPHICS_TONEMAPRENDERER

