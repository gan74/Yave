/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#include "GraphicPipeline.h"
#include "Material.h"

#include <yave/device/Device.h>

namespace yave {

GraphicPipeline::GraphicPipeline(const MaterialTemplate* mat, vk::Pipeline pipeline, vk::PipelineLayout layout) :
		DeviceLinked(mat->device()),
		_pipeline(pipeline),
		_layout(layout)  {
}

GraphicPipeline::~GraphicPipeline() {
	destroy(_pipeline);
	destroy(_layout);
}

GraphicPipeline::GraphicPipeline(GraphicPipeline&& other) {
	swap(other);
}

GraphicPipeline& GraphicPipeline::operator=(GraphicPipeline&& other) {
	swap(other);
	return *this;
}

void GraphicPipeline::swap(GraphicPipeline& other) {
	DeviceLinked::swap(other);
	std::swap(_pipeline, other._pipeline);
	std::swap(_layout, other._layout);
}

VkPipeline GraphicPipeline::vk_pipeline() const {
	return _pipeline;
}

VkPipelineLayout GraphicPipeline::vk_pipeline_layout() const {
	return _layout;
}

}
