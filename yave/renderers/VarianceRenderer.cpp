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

#include "VarianceRenderer.h"

#include <y/io/File.h>

namespace yave {

static ComputeShader create_variance_shader(DevicePtr dptr) {
	return ComputeShader(dptr, SpirVData::from_file(io::File::open("variance.comp.spv").expected("Unable to open SPIR-V file.")));
}


VarianceRenderer::VarianceRenderer(const Ptr<DepthRenderer>& buffer, u32 half_kernel_size) :
		BufferRenderer(buffer),
		_depth(buffer),
		_h(create_variance_shader(device()), half_kernel_size),
		_v(create_variance_shader(device()), -i32(half_kernel_size)),
		_variance_h(device(), ImageFormat(variance_format), size()),
		_variance(device(), ImageFormat(variance_format), size()),
		_descriptor_h(device(), {Binding(buffer->depth()), Binding(StorageView(_variance_h))}),
		_descriptor_v(device(), {Binding(TextureView(_variance_h)), StorageView(_variance)}) {
}


TextureView VarianceRenderer::depth_variance() const {
	return _variance;
}

void VarianceRenderer::build_frame_graph(RenderingNode<result_type>& node, CmdBufferRecorder<>& recorder) {
	node.add_dependency(_depth, recorder);

	node.set_func([=, &recorder]() -> result_type {
		recorder.dispatch_size(_h, size(), {_descriptor_h});
#warning VarianceRenderer barriers
		recorder.barriers({}, {ImageBarrier(_variance_h)}, PipelineStage::ComputeBit, PipelineStage::ComputeBit);
		recorder.dispatch_size(_v, size(), {_descriptor_v});

			return _variance;
		});
}

}
