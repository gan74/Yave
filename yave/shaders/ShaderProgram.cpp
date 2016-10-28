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

#include <spirv_cross/spirv.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

namespace yave {

template<typename M>
static void merge(M& into, const M& o) {
	into.insert(o.begin(), o.end());
}

static auto module_type(const spirv_cross::CompilerGLSL& compiler) {
	switch(compiler.get_execution_model()) {
		case spv::ExecutionModelVertex:
			return vk::ShaderStageFlagBits::eVertex;
		case spv::ExecutionModelFragment:
			return vk::ShaderStageFlagBits::eFragment;
		case spv::ExecutionModelGeometry:
			return vk::ShaderStageFlagBits::eGeometry;
		default:
			break;
	}
	log_msg("Unknown shader stage, setting to \"all\"", LogType::Warning);
	return vk::ShaderStageFlagBits::eAll;
}

/*struct Resources {
	vk::DescriptorType type;
	u32 set;
	u32 binding;
};*/


static vk::DescriptorSetLayoutBinding create_binding(u32 index, vk::DescriptorType type) {
	return vk::DescriptorSetLayoutBinding()
			.setBinding(index)
			.setDescriptorCount(1)
			.setDescriptorType(type)
			.setStageFlags(vk::ShaderStageFlagBits::eAll)
		;
}

static auto create_bindings(const spirv_cross::CompilerGLSL& compiler, const std::vector<spirv_cross::Resource>& resources, vk::DescriptorType type) {
	auto bindings = std::unordered_map<u32, core::Vector<vk::DescriptorSetLayoutBinding>>();
	for(auto r : resources) {
		auto id = r.id;
		bindings[compiler.get_decoration(id, spv::DecorationDescriptorSet)] << create_binding(compiler.get_decoration(id, spv::DecorationBinding), type);
	}
	return bindings;
}


Y_TODO(pool DescriptorSetLayouts !)

ShaderProgram::ShaderProgram(DevicePtr dptr, const core::Vector<SpirVData>& modules) /*: _modules(modules), _layouts(compute_layouts(_modules))*/ {
	auto layout_bindings = std::unordered_map<u32, core::Vector<vk::DescriptorSetLayoutBinding>>();
	for(const auto& mod : modules) {
		spirv_cross::CompilerGLSL compiler(std::vector<u32>(mod.data(), mod.data() + mod.size() / 4));
		_modules[module_type(compiler)] = ShaderModule(dptr, mod);

		auto resources = compiler.get_shader_resources();
		merge(layout_bindings, create_bindings(compiler, resources.uniform_buffers, vk::DescriptorType::eUniformBuffer));
		merge(layout_bindings, create_bindings(compiler, resources.sampled_images, vk::DescriptorType::eCombinedImageSampler));
	}
	u32 max_set = 0;
	core::range(layout_bindings).foreach([&](const auto& p) { max_set = std::max(max_set, p.first); });

	_layouts = core::Vector<vk::DescriptorSetLayout>(max_set + 1, VK_NULL_HANDLE);
	for(const auto& binding : layout_bindings) {
		_layouts[binding.first] = dptr->create_descriptor_set_layout(binding.second);
	}

	log_msg("shader descriptor layout count = "_s + _layouts.size());
}

core::Vector<vk::PipelineShaderStageCreateInfo> ShaderProgram::get_vk_pipeline_stage_info() const {
	core::Vector<vk::PipelineShaderStageCreateInfo> stage_create_infos;
	for(const auto& mod : _modules) {
		stage_create_infos << vk::PipelineShaderStageCreateInfo()
				.setModule(mod.second.get_vk_shader_module())
				.setStage(mod.first)
				.setPName("main")
			;
	}
	return stage_create_infos;
}

const core::Vector<vk::DescriptorSetLayout>& ShaderProgram::get_descriptor_layouts() const {
	return _layouts;
}

}
