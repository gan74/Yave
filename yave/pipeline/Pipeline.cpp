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

#include "Pipeline.h"

#include <unordered_set>

namespace yave {

static void find_nodes(Node::NodePtr& node, std::unordered_set<Node*>& nodes) {
	nodes.insert(node.as_ptr());
	for(auto& n : node->dependencies()) {
		find_nodes(n, nodes);
	}
}

Pipeline::Pipeline(Node::NodePtr root) : _root(std::move(root)) {
}

void Pipeline::process(concurrent::WorkGroup& worker, const FrameToken& token, CmdBufferRecorder<>& recorder) {
	std::unordered_set<Node*> nodes;
	find_nodes(_root, nodes);

	while(!nodes.empty()) {
		auto it = std::find_if(nodes.begin(), nodes.end(), [&nodes](auto& n) {
			auto deps = n->dependencies();
			return std::none_of(deps.begin(), deps.end(), [&nodes](auto& d) {
				return nodes.find(d.as_ptr()) != nodes.end();
			});
		});

		if(it == nodes.end()) {
			fatal("Unable to find processable node in pipeline.");
		}

		(*it)->process(token, recorder);

		nodes.erase(it);
	}
}

}
