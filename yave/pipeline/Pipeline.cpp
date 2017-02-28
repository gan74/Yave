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

namespace yave {
using SecCmdBuffer = RecordedCmdBuffer<CmdBufferUsage::Secondary>;
using SubRendererResults = core::Vector<SecCmdBuffer>;
using Recorder = CmdBufferRecorder<>;


static void process_node(Node* node, const FrameToken& token, SubRendererResults& results, Recorder& recorder);
static void process_node(SubRenderer* node, const FrameToken& token, SubRendererResults& results, Recorder& recorder);
static void process_node(Renderer* node, const FrameToken& token, SubRendererResults& results, Recorder& recorder);


template<usize N, typename... Args>
struct DependencyProcessor {
	template<typename... Fwd>
	static void process(std::tuple<Args...>& deps, Fwd&&... fwd) {
		auto d = std::get<N - 1>(deps);
		if(d) {
			process_node(d, std::forward<Fwd>(fwd)...);
		}
		DependencyProcessor<N - 1, Args...>::process(deps, std::forward<Fwd>(fwd)...);
	}
};

template<typename... Args>
struct DependencyProcessor<0, Args...> {
	template<typename... Fwd>
	static void process(std::tuple<Args...>&, Fwd&&...) {
	}
};

template<typename... Args, typename... Fwd>
void process_dependencies(core::Vector<std::tuple<Args...>> deps, Fwd&&... fwd) {
	for(auto& d : deps) {
		DependencyProcessor<sizeof...(Args), Args...>::process(d, std::forward<Fwd>(fwd)...);
	}
}



void process_node(Node* node, const FrameToken& token, SubRendererResults& results, Recorder& recorder) {
	process_dependencies(node->dependencies(), token, results, recorder);
	node->process(token);
}


void process_node(SubRenderer* node, const FrameToken& token, SubRendererResults& results, Recorder& recorder) {
	process_dependencies(node->dependencies(), token, results, recorder);
	results << node->process(token);
}


void process_node(Renderer* node, const FrameToken& token, SubRendererResults&, Recorder& recorder) {
	SubRendererResults results;
	process_dependencies(node->dependencies(), token, results, recorder);
	node->process(token, recorder, results);
}



void process_root(Renderer* root, const FrameToken& token, Recorder& recorder) {
	SubRendererResults _;
	process_node(root, token, _, recorder);
}

}
