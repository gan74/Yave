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

#include "ComputeProgram.h"

#include <yave/device/Device.h>

#include <numeric>

namespace yave {

ComputeProgram::ComputeProgram(const ComputeShader& comp, const SpecializationData& data) : DeviceLinked(comp.device()), _local_size(comp.local_size()) {
	const auto& bindings = comp.bindings();

	const u32 max_set = std::accumulate(bindings.begin(), bindings.end(), 0, [](u32 max, const auto& p) { return std::max(max, p.first); });

	auto layouts = core::Vector<VkDescriptorSetLayout>(max_set + 1, VkDescriptorSetLayout());
	for(const auto& binding : bindings) {
		layouts[binding.first] = device()->descriptor_set_layout(binding.second).vk_descriptor_set_layout();
	}

	VkPipelineLayoutCreateInfo layout_create_info = vk_struct();
	{
		layout_create_info.setLayoutCount = layouts.size();
		layout_create_info.pSetLayouts = layouts.data();
		layout_create_info.pushConstantRangeCount = comp.vk_push_constants().size();
		layout_create_info.pPushConstantRanges = comp.vk_push_constants().data();
	}

	vk_check(vkCreatePipelineLayout(device()->vk_device(), &layout_create_info, nullptr, &_layout.get()));


	if(data.size() && data.size() != comp.specialization_data_size()) {
		y_fatal("Incompatible specialization data.");
	}

	auto entries = data.size() ? comp.specialization_entries() : core::Span<VkSpecializationMapEntry>();
	VkSpecializationInfo spec_info = {};
	{
		spec_info.mapEntryCount = entries.size();
		spec_info.pMapEntries = entries.data();
		spec_info.dataSize = data.size();
		spec_info.pData = data.data();
	}

	VkPipelineShaderStageCreateInfo stage = vk_struct();
	{
		stage.module = comp.vk_shader_module();
		stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
		stage.pName = "main";
		stage.pSpecializationInfo = &spec_info;
	}

	VkComputePipelineCreateInfo create_info = vk_struct();
	{
		create_info.layout = _layout;
		create_info.stage = stage;
	}

	vk_check(vkCreateComputePipelines(device()->vk_device(), vk_null(), 1, &create_info, nullptr, &_pipeline.get()));
}

ComputeProgram::~ComputeProgram() {
	destroy(_layout);
	destroy(_pipeline);
}

const math::Vec3ui& ComputeProgram::local_size() const {
	return _local_size;
}

usize ComputeProgram::thread_count() const {
	return _local_size.x() * _local_size.y() * _local_size.z();
}

VkPipeline ComputeProgram::vk_pipeline() const {
	return _pipeline;
}

VkPipelineLayout ComputeProgram::vk_pipeline_layout() const {
	return _layout;
}

}
