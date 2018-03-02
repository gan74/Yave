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

namespace yave {

void RenderingPipeline::process_nodes() {
	Y_LOG_PERF("pipeline");
	while(!_nodes.empty()) {
		bool dead = true;
		for(auto it = _nodes.begin(); it != _nodes.end(); ++it) {
			auto& data = it->second;
			if(std::all_of(data.dependencies.begin(), data.dependencies.end(), [&](const auto& d) { return _done.find(d) != _done.end(); })) {
				auto map_node = _nodes.extract(it);

				auto node = std::move(map_node.key());
				auto data = std::move(map_node.mapped());

				//log_msg("- " + type_name(*node));
				data.process();

				_done.insert(std::make_pair(std::move(node), std::move(data)));

				dead = false;
				break;
			}
		}
		if(dead) {
			fatal("Unable to find a node to process.");
		}
	}
}

}
