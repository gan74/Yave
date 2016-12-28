/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#include "ComputeProgram.h"

#include <yave/Device.h>

#include <numeric>

namespace yave {

ComputeProgram::ComputeProgram(const ShaderModule<ShaderType::Compute> &comp) : DeviceLinked(comp.device()) {
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

vk::Pipeline ComputeProgram::vk_pipeline() const {
	return _pipeline;
}

}
