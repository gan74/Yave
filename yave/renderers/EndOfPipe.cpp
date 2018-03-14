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

#include "EndOfPipe.h"
#include "GBufferRenderer.h"

#include <y/io/File.h>

namespace yave {
namespace experimental {

static ComputeShader create_end_of_pipe_shader(DevicePtr dptr) {
	return ComputeShader(dptr, SpirVData::from_file(io::File::open("correction.comp.spv").expected("Unable to open SPIR-V file.")));
}

EndOfPipe::EndOfPipe(const Ptr<TiledDeferredRenderer>& renderer) :
		Renderer(renderer->device()),
		_renderer(renderer),
		_correction_program(create_end_of_pipe_shader(device())) {
}

void EndOfPipe::build_frame_graph(FrameGraphNode& frame_graph) {
	frame_graph.schedule(_renderer);
}

void EndOfPipe::render(CmdBufferRecorder<>& recorder, const FrameToken& token) {
	auto region = recorder.region("EndOfPipe::render");

	auto size = token.image_view.size();
#warning assuming TiledDeferredRenderer !!!
	auto image = dynamic_cast<TiledDeferredRenderer*>(_renderer.as_ptr())->lighting();
	recorder.dispatch_size(_correction_program, size, {create_descriptor_set(token.image_view, image)});
}

const DescriptorSet& EndOfPipe::create_descriptor_set(const StorageView& out, const TextureView& in) {
	if(out.size() != in.size()) {
		fatal("Invalid output image size.");
	}

	auto it = _output_sets.find(out.vk_view());
	if(it == _output_sets.end()) {
		it = _output_sets.insert(std::pair(out.vk_view(), DescriptorSet(_renderer->device(), {
				Binding(in),
				Binding(out)
			}))).first;
	}
	return it->second;
}



ScreenEndOfPipe::ScreenEndOfPipe(const Ptr<SecondaryRenderer>& renderer) :
		Renderer(renderer->device()),
		_renderer(renderer) {
}

void ScreenEndOfPipe::build_frame_graph(FrameGraphNode& frame_graph) {
	frame_graph.schedule(_renderer);
}

void ScreenEndOfPipe::render(CmdBufferRecorder<>& recorder, const FrameToken& token) {
	auto region = recorder.region("ScreenEndOfPipe::render");

	auto pass = recorder.bind_framebuffer(create_framebuffer(token.image_view));
	_renderer->render(pass, token);
}

const Framebuffer& ScreenEndOfPipe::create_framebuffer(const ColorAttachmentView& out) {
	auto it = _output_framebuffers.find(out.vk_view());
	if(it == _output_framebuffers.end()) {
		it = _output_framebuffers.insert(std::pair(out.vk_view(), Framebuffer(device(), {out}))).first;
	}
	return it->second;
}

}
}
