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

namespace yave {

class RenderingNodeProcessor;
class RenderingPipeline;

class Node : NonCopyable {
	public:
		virtual ~Node() {
		}
};

class SecondaryRenderer : public DeviceLinked, public Node {
	public:
		SecondaryRenderer(DevicePtr dptr) : DeviceLinked(dptr) {
		}

		virtual RecordedCmdBuffer<CmdBufferUsage::Secondary> process(const FrameToken&, const Framebuffer&) = 0;
};


class Renderer : public DeviceLinked, public Node {
	public:
		Renderer(DevicePtr dptr) : DeviceLinked(dptr) {
		}

		virtual void process(const FrameToken&, CmdBufferRecorder<>&) = 0;
};

class BufferRenderer : public DeviceLinked, public Node {
	public:
		BufferRenderer(DevicePtr dptr) : DeviceLinked(dptr) {
		}

		virtual TextureView process(const FrameToken&, CmdBufferRecorder<>&) = 0;
};

class EndOfPipeline : public DeviceLinked, public Node {
	public:
		EndOfPipeline(DevicePtr dptr) : DeviceLinked(dptr) {
		}

		virtual void process(const FrameToken&, CmdBufferRecorder<>&) = 0;
};

}

#endif // YAVE_RENDERERS_RENDERERS_H
