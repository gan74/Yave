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

#include <y/core/Chrono.h>

#include <unordered_map>
#include <unordered_set>
#include <shared_mutex>

namespace yave {

enum ProcessingState {
	ToDo,
	Processing,
	Done
};

static void find_nodes(Node::NodePtr& node, std::unordered_map<Node*, ProcessingState>& nodes) {
	nodes.insert(std::make_pair(node.as_ptr(), ToDo));
	for(auto& n : node->dependencies()) {
		find_nodes(n, nodes);
	}
}

static void find_nodes(Node::NodePtr& node, std::unordered_set<Node*>& nodes) {
	nodes.insert(node.as_ptr());
	for(auto& n : node->dependencies()) {
		find_nodes(n, nodes);
	}
}

Pipeline::Pipeline(Node::NodePtr root) : _root(std::move(root)) {
}

void Pipeline::process(concurrent::WorkGroup& worker, const FrameToken& token, CmdBufferRecorder<>& recorder) {
	/*std::unordered_map<Node*, ProcessingState> nodes;
	find_nodes(_root, nodes);

	std::mutex lock;
	std::condition_variable notify;

	core::Vector<std::future<void>> futures;
	for(usize i = 0; i != worker.concurrency(); ++i) {
		futures << worker.schedule([&]() {
			while(true) {
				auto it = nodes.end();
				{
					std::unique_lock<decltype(lock)> lk(lock);
					it = std::find_if(nodes.begin(), nodes.end(), [&](const auto& w) {
							if(w.second != ToDo) {
								return false;
							}
							auto deps = w.first->dependencies();
							return std::all_of(deps.begin(), deps.end(), [&](auto& d) { return nodes[d.as_ptr()] == Done; });
						});
					if(it == nodes.end()) {
						if(std::find_if(nodes.begin(), nodes.end(), [&](const auto& w) { return w.second == ToDo; }) == nodes.end()) {
							return;
						}
						notify.wait(lk);
						continue;
					}
					(*it).second = Processing;
				}
				(*it).first->process(token, recorder);
				{
					std::lock_guard<decltype(lock)> _(lock);
					(*it).second = Done;
				}
				notify.notify_all();
			}
		});
	}

	std::for_each(futures.begin(), futures.end(), [](auto& f) { f.wait(); });*/


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

		{
			core::DebugTimer _(type_name(**it), core::Duration::milliseconds(2));
			(*it)->process(token, recorder);
		}

		nodes.erase(it);
	}
}

}
