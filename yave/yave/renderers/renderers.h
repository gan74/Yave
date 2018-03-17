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
#ifndef  YAVE_RENDERERS_RENDERERS_H
#define  YAVE_RENDERERS_RENDERERS_H

#include "RenderingPipeline.h"

namespace yave {

class Node : NonCopyable {
	public:
		template<typename T>
		using Ptr = core::Arc<T>;

		virtual ~Node() {
		}

	protected:
		friend class RenderingPipeline;

		virtual void build_frame_graph(FrameGraphNode& node) = 0;
		virtual void prepare() {
		}

	private:
};

class SecondaryRenderer : public Node, public DeviceLinked {
	public:
		SecondaryRenderer(DevicePtr dptr) : DeviceLinked(dptr) {
		}

		virtual void render(RenderPassRecorder& recorder, const FrameToken& token) = 0;

	protected:
		friend class RenderingPipeline;

		virtual void pre_render(CmdBufferRecorder<>&, const FrameToken&) {
		}


};

class Renderer : public Node, public DeviceLinked {
	public:
		Renderer(DevicePtr dptr) : DeviceLinked(dptr) {
		}

		virtual TextureView output() const = 0;

	protected:
		friend class RenderingPipeline;

		virtual void pre_render(CmdBufferRecorder<>&, const FrameToken&) {
		}

		virtual void render(CmdBufferRecorder<>& recorder, const FrameToken& token) = 0;
};

class EndOfPipe : public Node, public DeviceLinked {
	public:
		EndOfPipe(DevicePtr dptr) : DeviceLinked(dptr) {
		}

	protected:
		friend class RenderingPipeline;

		virtual void render(CmdBufferRecorder<>& recorder, const FrameToken& token) = 0;
};

}

#endif //  YAVE_RENDERERS_RENDERERS_H
