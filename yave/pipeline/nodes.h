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
#ifndef YAVE_PIPELINE_NODES_H
#define YAVE_PIPELINE_NODES_H

#include <yave/yave.h>
#include <yave/swapchain/FrameToken.h>

namespace yave {

class RenderingNodeProcessor;
class RenderingNode;

class NodeBase : NonCopyable {
	public:
		virtual ~NodeBase() {
		}

	protected:
		friend class RenderingNode;

		virtual void compute_dependencies(const FrameToken&, RenderingNode&) = 0;
};



class Node : public NodeBase {
	protected:
		friend class RenderingNodeProcessor;

		virtual void process(const FrameToken&) = 0;
};


class SecondaryRenderer : public DeviceLinked, public NodeBase {
	public:
		SecondaryRenderer(DevicePtr dptr) : DeviceLinked(dptr) {
		}

	protected:
		friend class RenderingNodeProcessor;

		virtual void process(const FrameToken&, CmdBufferRecorder<CmdBufferUsage::Secondary>&&) = 0;
};


class Renderer : public DeviceLinked, public NodeBase {
	public:
		Renderer(DevicePtr dptr) : DeviceLinked(dptr) {
		}

		virtual TextureView view() const = 0;

		math::Vec2ui size() const {
			return view().size();
		}

	protected:
		friend class RenderingNodeProcessor;

		virtual void process(const FrameToken&, CmdBufferRecorder<>&) = 0;
};

class EndOfPipeline : public DeviceLinked, public NodeBase {
	public:
		EndOfPipeline(DevicePtr dptr) : DeviceLinked(dptr) {
		}

	protected:
		friend class RenderingNodeProcessor;

		virtual void process(const FrameToken&, CmdBufferRecorder<>&) = 0;
};

}

#endif // YAVE_PIPELINE_NODES_H
