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

#include "SimpleEndOfPipe.h"

namespace yave {

SimpleEndOfPipe::SimpleEndOfPipe(const core::ArrayView<Ptr<SecondaryRenderer>>& renderers) :
		EndOfPipe(renderers[0]->device()),
		_renderers(renderers.begin(), renderers.end()) {
}

void SimpleEndOfPipe::build_frame_graph(FrameGraphNode& frame_graph) {
	for(const auto& renderer : _renderers) {
		frame_graph.schedule(renderer);
	}
}

void SimpleEndOfPipe::render(CmdBufferRecorder<>& recorder, const FrameToken& token) {
	auto region = recorder.region("SimpleEndOfPipe::render");

	auto pass = recorder.bind_framebuffer(create_framebuffer(token.image_view));

	for(const auto& renderer : _renderers) {
		renderer->render(pass, token);
	}
}

const Framebuffer& SimpleEndOfPipe::create_framebuffer(const ColorAttachmentView& out) {
	auto it = _output_framebuffers.find(out.vk_view());
	if(it == _output_framebuffers.end()) {
		it = _output_framebuffers.insert(std::pair(out.vk_view(), Framebuffer(device(), {out}))).first;
	}
	return it->second;
}

}
