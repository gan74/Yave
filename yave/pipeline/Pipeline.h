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
#ifndef YAVE_PIPELINE_PIPELINE_H
#define YAVE_PIPELINE_PIPELINE_H

#include <yave/commands/RecordedCmdBuffer.h>
#include <yave/swapchain/FrameToken.h>

namespace yave {

struct Node : NonCopyable {
	using Dependency = std::tuple<Node*>;

	virtual void process(const FrameToken&) = 0;
	virtual core::Vector<Dependency> dependencies() {
		return {};
	}

	virtual ~Node() {}
};

struct SubRenderer : NonCopyable {
	using Dependency = std::tuple<Node*>;

	virtual RecordedCmdBuffer<CmdBufferUsage::Secondary> process(const FrameToken&) = 0;
	virtual core::Vector<Dependency> dependencies() {
		return {};
	}

	virtual ~SubRenderer() {}
};

struct Renderer : NonCopyable {
	using SecCmdBuffer = RecordedCmdBuffer<CmdBufferUsage::Secondary>;
	using SubRendererResults = core::Vector<SecCmdBuffer>;
	using Dependency = std::tuple<Node*, SubRenderer*, Renderer*>;

	virtual void process(const FrameToken&, CmdBufferRecorder<>&, const SubRendererResults&) = 0;
	virtual core::Vector<Dependency> dependencies() {
		return {};
	}

	virtual ~Renderer() {}
};

void process_root(Renderer* root, const FrameToken& token, CmdBufferRecorder<>& recorder);

}

#endif // YAVE_PIPELINE_PIPELINE_H
