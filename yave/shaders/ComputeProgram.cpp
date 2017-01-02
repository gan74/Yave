/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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

#include "ComputeProgram.h"

#include <yave/Device.h>

#include <numeric>

namespace yave {

ComputeProgram::ComputeProgram(const ComputeShader& comp) : DeviceLinked(comp.device()) {
	const auto& bindings = comp.bindings();

	u32 max_set = std::accumulate(bindings.begin(), bindings.end(), 0, [](u32 max, const auto& p) { return std::max(max, p.first); });

	auto layouts = core::Vector<vk::DescriptorSetLayout>(max_set + 1, VK_NULL_HANDLE);
	for(const auto& binding : bindings) {
		layouts[binding.first] = device()->create_descriptor_set_layout(binding.second);
	}
	_layout = device()->vk_device().createPipelineLayout(vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(u32(layouts.size()))
			.setPSetLayouts(layouts.begin())
		);

	auto stage = vk::PipelineShaderStageCreateInfo()
			.setModule(comp.vk_shader_module())
			.setStage(vk::ShaderStageFlagBits::eCompute)
			.setPName("main")
		;

	_pipeline = device()->vk_device().createComputePipeline(VK_NULL_HANDLE, vk::ComputePipelineCreateInfo()
			.setLayout(_layout)
			.setStage(stage)
		);
}

ComputeProgram::~ComputeProgram() {
	destroy(_layout);
	destroy(_pipeline);
}

ComputeProgram::ComputeProgram(ComputeProgram&& other) : ComputeProgram() {
	swap(other);
}

ComputeProgram& ComputeProgram::operator=(ComputeProgram&& other) {
	swap(other);
	return *this;
}

void ComputeProgram::swap(ComputeProgram& other) {
	DeviceLinked::swap(other);
	std::swap(_layout, other._layout);
	std::swap(_pipeline, other._pipeline);
}

vk::Pipeline ComputeProgram::vk_pipeline() const {
	return _pipeline;
}

vk::PipelineLayout ComputeProgram::vk_pipeline_layout() const {
	return _layout;
}

}
