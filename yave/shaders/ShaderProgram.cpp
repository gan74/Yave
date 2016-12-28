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

#include "ShaderProgram.h"
#include <yave/Device.h>

#include <unordered_map>
#include <numeric>

#include <spirv_cross/spirv.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

namespace yave {

static void merge(std::unordered_map<u32, core::Vector<vk::DescriptorSetLayoutBinding>>& into, const std::unordered_map<u32, core::Vector<vk::DescriptorSetLayoutBinding>>& o) {
	into.insert(o.begin(), o.end());
#if 0
	for(const auto& e : o) {
		auto& v = into[e.first];
		for(vk::DescriptorSetLayoutBinding binding : e.second) {

			binding.stageFlags = vk::ShaderStageFlagBits::eAll;
			v << binding;

			/*it = std::find_if(v.begin(), v.end(), [=](const vk::DescriptorSetLayoutBinding& b) {
					return std::tie(b.binding, b.descriptorType, b.descriptorCount, b.pImmutableSamplers) ==
						   std::tie(binding.binding, binding.descriptorType, binding.descriptorCount, binding.pImmutableSamplers);
				});
			if(it != v.end()) {
				it->stageFlags |= binding.stageFlags;
			} else {
				v << binding;
			}*/
		}
	}
#endif
}

static auto create_stage_info(core::Vector<vk::PipelineShaderStageCreateInfo>& stages, const ShaderModuleBase& mod) {
	if(mod.vk_shader_module()) {
		stages << vk::PipelineShaderStageCreateInfo()
				.setModule(mod.vk_shader_module())
				.setStage(vk::ShaderStageFlagBits(mod.type()))
				.setPName("main")
			;
	}
}

ShaderProgram::ShaderProgram(const FragmentShader& frag, const VertexShader& vert, const GeometryShader& geom) : DeviceLinked(common_device(frag, vert, geom)) {
	merge(_bindings, frag.bindings());
	merge(_bindings, vert.bindings());
	merge(_bindings, geom.bindings());

	create_stage_info(_stages, frag);
	create_stage_info(_stages, vert);
	create_stage_info(_stages, geom);

	u32 max_set = std::accumulate(_bindings.begin(), _bindings.end(), 0, [](u32 max, const auto& p) { return std::max(max, p.first); });

	_layouts = core::Vector<vk::DescriptorSetLayout>(max_set + 1, VK_NULL_HANDLE);
	for(const auto& binding : _bindings) {
		_layouts[binding.first] = device()->create_descriptor_set_layout(binding.second);
	}

	log_msg("shader descriptor layout count = "_s + _layouts.size());
}

core::Vector<vk::PipelineShaderStageCreateInfo> ShaderProgram::vk_pipeline_stage_info() const {
	return _stages;
}

const core::Vector<vk::DescriptorSetLayout>& ShaderProgram::descriptor_layouts() const {
	return _layouts;
}

}
