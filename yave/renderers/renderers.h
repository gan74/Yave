/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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
#ifndef YAVE_RENDERERS_RENDERERS_H
#define YAVE_RENDERERS_RENDERERS_H

#include <yave/yave.h>
#include <yave/swapchain/FrameToken.h>

#include "RenderingPipeline.h"
#include "BufferRenderer.h"

namespace yave {

class SecondaryRenderer : public Node, public DeviceLinked {
	public:
		using result_type = RecordedCmdBuffer<CmdBufferUsage::Secondary>;

		SecondaryRenderer(DevicePtr dptr) : DeviceLinked(dptr) {
		}

		virtual void build_frame_graph(RenderingNode<result_type>&, const Framebuffer&) = 0;
};


class Renderer : public Node, public DeviceLinked {
	public:
		using result_type = void;

		Renderer(DevicePtr dptr) : DeviceLinked(dptr) {
		}

		virtual void build_frame_graph(RenderingNode<result_type>&, CmdBufferRecorder<>&) = 0;
};

class EndOfPipeline : public Node,  public DeviceLinked {
	public:
		using result_type = void;

		EndOfPipeline(DevicePtr dptr) : DeviceLinked(dptr) {
		}

		virtual void build_frame_graph(RenderingNode<result_type>&, CmdBufferRecorder<>&) = 0;
};

}

#endif // YAVE_RENDERERS_RENDERERS_H
