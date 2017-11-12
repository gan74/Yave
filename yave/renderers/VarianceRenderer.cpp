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

#include "VarianceRenderer.h"

#include <y/io/File.h>

namespace yave {

static ComputeShader create_variance_shader(DevicePtr dptr) {
	return ComputeShader(dptr, SpirVData::from_file(io::File::open("variance.comp.spv").expected("Unable to open SPIR-V file.")));
}

VarianceRenderer::VarianceRenderer(const Ptr<BufferRenderer>& buffer) :
		BufferRenderer(buffer),
		_buffer(buffer),
		_program(create_variance_shader(device())),
		_variance(device(), ImageFormat(variance_format), size()),
		_descriptor_set(device(), {Binding(buffer->depth())}) {
}


TextureView VarianceRenderer::depth_variance() const {
	return _variance;
}

void VarianceRenderer::build_frame_graph(RenderingNode<result_type>& node, CmdBufferRecorder<>& recorder) {
	node.add_dependency(_buffer, recorder);

	node.set_func([=, &recorder]() -> result_type {
			recorder.dispatch_size(_program, size(), {_descriptor_set});

			return _variance;
		});
}

}
