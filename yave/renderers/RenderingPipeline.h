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
#ifndef  YAVE_RENDERERS_RENDERINGPIPELINE_H
#define  YAVE_RENDERERS_RENDERINGPIPELINE_H

#include <yave/commands/CmdBuffer.h>
#include <yave/swapchain/FrameToken.h>

#include <y/concurrent/Arc.h>

#include <unordered_map>

namespace yave {

class Node;

class RenderingPipeline : NonCopyable {
	public:
		template<typename T>
		using Ptr = core::Arc<T>;

	private:
		using NodePtr = RenderingPipeline::Ptr<Node>;
		using NodeDependencyMap = std::unordered_map<NodePtr, core::Vector<NodePtr>>;

	public:
		RenderingPipeline(const NodePtr& root);

		void render(CmdBufferRecorder<>& recorder, const FrameToken& token);

	private:

		void add_node(NodeDependencyMap& dependencies, const NodePtr& node);

		NodePtr _root;
};

class FrameGraphNode {
	public:
		template<typename T>
		using Ptr = RenderingPipeline::Ptr<T>;

		FrameGraphNode(core::Vector<Ptr<Node>>& dependencies); // hide somehow

		void schedule(const Ptr<Node>& node); // change name ???

	private:
		core::Vector<Ptr<Node>>& _dependencies;
};

}

#endif //  YAVE_RENDERERS_RENDERINGPIPELINE_H
