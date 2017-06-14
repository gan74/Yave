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

#include "pipeline.h"

#include <unordered_set>

namespace yave {

RenderingNodeProcessor::RenderingNodeProcessor(const FrameToken& token, NotOwner<Node*> node) :
		_type(Type::Node),
		_token(token),
		_node(node) {
}

RenderingNodeProcessor::RenderingNodeProcessor(const FrameToken& token, NotOwner<SecondaryRenderer*> node, const Framebuffer& framebuffer) :
		_type(Type::Secondary),
		_token(token),
		_node(node),
		_framebuffer(&framebuffer) {
}

RenderingNodeProcessor::RenderingNodeProcessor(const FrameToken& token, NotOwner<Renderer*> node) :
		_type(Type::Primary),
		_token(token),
		_node(node) {
}

RenderingNodeProcessor::RenderingNodeProcessor(const FrameToken& token, NotOwner<EndOfPipeline*> node) :
		_type(Type::End),
		_token(token),
		_node(node) {
}

NodeBase* RenderingNodeProcessor::node() const {
	return _node;
}

const FrameToken& RenderingNodeProcessor::token() const {
	return _token;
}

bool RenderingNodeProcessor::operator==(const RenderingNodeProcessor& other) const {
	return _node == other._node && _token == other._token && _framebuffer == other._framebuffer;
}

bool RenderingNodeProcessor::operator!=(const RenderingNodeProcessor& other) const {
	return !operator==(other);
}

usize RenderingNodeProcessor::Hash::operator()(const RenderingNodeProcessor& data) const {
	return hash_range<std::initializer_list<const void*>>({reinterpret_cast<const void*>(data._framebuffer), reinterpret_cast<const void*>(data._node)});
}


void RenderingNodeProcessor::operator()(DevicePtr dptr, CmdBufferRecorder<>& recorder) const {
	switch(_type) {
		case Type::Node:
			dynamic_cast<Node*>(_node)->process(_token);
		break;

		case Type::Secondary:
			dynamic_cast<SecondaryRenderer*>(_node)->process(_token, CmdBufferRecorder<CmdBufferUsage::Secondary>(dptr->create_secondary_cmd_buffer(), *_framebuffer));
		break;

		case Type::Primary:
			dynamic_cast<Renderer*>(_node)->process(_token, recorder);
		break;

		case Type::End:
			dynamic_cast<EndOfPipeline*>(_node)->process(_token, recorder);
		break;

		default:
			fatal("Unknown dependency type.");
	}
}



RenderingPipeline::RenderingPipeline(Processor& node, DependencyGraph& graph) : _dependencies(graph[node]), _graph(graph) {
	node.node()->build_frame_graph(node.token(), *this);
}

void RenderingPipeline::add_node_dependency(Processor&& node) {
	_dependencies.push_back(node);
	if(_graph.find(node) == _graph.end()) {
		RenderingPipeline(node, _graph);
	}
}




RecordedCmdBuffer<> build_pipeline_command_buffer(const FrameToken& token, EndOfPipeline* pipeline) {
	using Processor = RenderingNodeProcessor;
	using DependencyGraph = std::unordered_map<Processor, core::Vector<Processor>, Processor::Hash>;


	DependencyGraph graph;
	Processor root(token, pipeline);

	RenderingPipeline(root, graph);

	DevicePtr dptr = pipeline->device();
	CmdBufferRecorder<> recorder = dptr->create_cmd_buffer();

	std::unordered_set<Processor, Processor::Hash> done;

	while(done.find(root) == done.end()) {
		bool dead_lock = true;
		for(auto& node : graph) {
			auto& dependencies = node.second;

			if(std::all_of(dependencies.begin(), dependencies.end(), [&](const auto& node) { return done.find(node) != done.end(); })) {
				node.first(dptr, recorder);
				dead_lock = false;
				done.insert(node.first);
				graph.erase(graph.find(node.first));
				break;
			}
		}
		if(dead_lock) {
			fatal("Invalid pipeline graph: unable to find any ready node.");
		}
	}

	return recorder.end();
}


}
