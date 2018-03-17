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

#include "RenderingPipeline.h"
#include "renderers.h"

namespace yave {

RenderingPipeline::RenderingPipeline(const NodePtr &root) : _root(root) {
}

void RenderingPipeline::add_node(NodeDependencyMap& dependencies, const NodePtr& node) {
	if(auto it = dependencies.find(node); it == dependencies.end()) {
		auto& deps = dependencies[node];
		FrameGraphNode frame_graph_node(deps);
		node->build_frame_graph(frame_graph_node);
		for(const auto& d : deps) {
			add_node(dependencies, d);
		}
	}
}

void RenderingPipeline::render(CmdBufferRecorder<>& recorder, const FrameToken& token) {
	NodeDependencyMap dependencies;
	add_node(dependencies, _root);

	auto ordered = core::vector_with_capacity<NodePtr>(dependencies.size());
	while(true) {
		auto it = std::find_if(dependencies.begin(), dependencies.end(), [](const auto& deps) { return deps.second.is_empty(); });
		if(it == dependencies.end()) {
			break;
		}

		ordered << it->first;
		for(auto& deps : dependencies) {
			if(auto d = std::find(deps.second.begin(), deps.second.end(), it->first); d != deps.second.end()) {
				deps.second.erase_unordered(d);
			}
		}
		dependencies.erase(it);
	}

	for(auto& node : ordered) {
		node->prepare();
	}

	{
		for(auto& node : ordered) {
			if(auto renderer = dynamic_cast<SecondaryRenderer*>(node.as_ptr())) {
				renderer->pre_render(recorder, token);
			}
		}

		for(auto& node : ordered) {
			if(auto renderer = dynamic_cast<Renderer*>(node.as_ptr())) {
				renderer->pre_render(recorder, token);
			}
		}
	}

	for(auto& node : ordered) {
		if(auto renderer = dynamic_cast<Renderer*>(node.as_ptr())) {
			renderer->render(recorder, token);
		}
	}
}

FrameGraphNode::FrameGraphNode(core::Vector<Ptr<Node>>& dependencies) : _dependencies(dependencies) {
}

void FrameGraphNode::schedule(const Ptr<Node>& node) {
	_dependencies << node;
}

}
