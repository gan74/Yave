/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef YAVE_RENDERERS_BUFFERRENDERER_H
#define YAVE_RENDERERS_BUFFERRENDERER_H

#include "BufferRendererType.h"
#include "RenderingPipeline.h"

namespace yave {

class BufferRenderer : public Node, public DeviceLinked {

	public:
		using result_type = TextureView;

#warning BufferRenderer barriers

		BufferRenderer(DevicePtr dptr, const math::Vec2& size) : DeviceLinked(dptr), _size(size) {
		}

		BufferRenderer(const Ptr<BufferRenderer>& renderer) : BufferRenderer(renderer->device(), renderer->size()) {
		}

		const math::Vec2ui& size() const {
			return _size;
		}

		virtual void build_frame_graph(RenderingNode<result_type>&, CmdBufferRecorder<>&) = 0;

		// TODO duck.
		virtual TextureView depth() const { return fatal("No depth."); }
		virtual TextureView albedo_metallic() const { return fatal("No albedo metallic."); }
		virtual TextureView normal_roughness() const { return fatal("No normal roughness."); }
		virtual TextureView depth_variance() const { return fatal("No depth variance."); }
		virtual TextureView lighting() const { return fatal("No lighting."); }

	private:
		math::Vec2ui _size;
};


}



#endif // BUFFERRENDERER_H
