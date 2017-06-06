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

#include "nodes.h"

namespace yave {

RecordedCmdBuffer<> build_pipeline_command_buffer(const FrameToken& token, EndOfPipeline* pipeline);

class RenderingNodeProcessor {
	enum class Type {
		Node,
		Secondary,
		Primary,
		End
	};

	public:
		RenderingNodeProcessor(const FrameToken& token, NotOwner<Node*> node);
		RenderingNodeProcessor(const FrameToken& token, NotOwner<SecondaryRenderer*> node, const Framebuffer& framebuffer);
		RenderingNodeProcessor(const FrameToken& token, NotOwner<Renderer*> node);
		RenderingNodeProcessor(const FrameToken& token, NotOwner<EndOfPipeline*> node);

		void operator()(DevicePtr dptr, CmdBufferRecorder<>& recorder) const;

		NodeBase* node() const;
		const FrameToken& token() const;

		bool operator==(const RenderingNodeProcessor& other) const;
		bool operator!=(const RenderingNodeProcessor& other) const;

		struct Hash {
			usize operator()(const RenderingNodeProcessor& data) const;
		};

	private:
		Type _type;
		FrameToken _token;

		NodeBase* _node = nullptr;
		const Framebuffer* _framebuffer = nullptr;
};


class RenderingNode : NonCopyable {
	using Processor = RenderingNodeProcessor;
	using DependencyGraph = std::unordered_map<Processor, core::Vector<Processor>, Processor::Hash>;

	public:
		template<typename... Args>
		void add_dependency(Args&&... args) {
			add_node_dependency(Processor(std::forward<Args>(args)...));
		}


	private:
		friend RecordedCmdBuffer<> build_pipeline_command_buffer(const FrameToken&, EndOfPipeline*);

		RenderingNode(Processor& node, DependencyGraph& graph);

	private:
		void add_node_dependency(Processor&& node);

		core::Vector<RenderingNodeProcessor>& _dependencies;
		DependencyGraph& _graph;
};






}

#endif // YAVE_PIPELINE_PIPELINE_H
