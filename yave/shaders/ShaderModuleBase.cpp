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

#include "ShaderModuleBase.h"

#include <yave/Device.h>

#include <spirv_cross/spirv.hpp>
#include <spirv_cross/spirv_cross.hpp>

namespace yave {

template<typename M>
static void merge(M& into, const M& o) {
	into.insert(o.begin(), o.end());
}

static vk::ShaderModule create_shader_module(DevicePtr dptr, const SpirVData& data) {
	if(data.is_empty()) {
		return VK_NULL_HANDLE;
	}
	return dptr->vk_device().createShaderModule(vk::ShaderModuleCreateInfo()
			.setCodeSize(data.size())
			.setPCode(data.data())
		);
}

static ShaderType module_type(const spirv_cross::Compiler& compiler) {
	switch(compiler.get_execution_model()) {
		case spv::ExecutionModelVertex:
			return ShaderType::Vertex;
		case spv::ExecutionModelFragment:
			return ShaderType::Fragment;
		case spv::ExecutionModelGeometry:
			return ShaderType::Geomery;

		case spv::ExecutionModelGLCompute:
		case spv::ExecutionModelKernel:
			return ShaderType::Compute;

		default:
			break;
	}
	return fatal("Unknown shader execution model");
}

static vk::DescriptorSetLayoutBinding create_binding(u32 index, ShaderType, vk::DescriptorType type) {
	return vk::DescriptorSetLayoutBinding()
			.setBinding(index)
			.setDescriptorCount(1)
			.setDescriptorType(type)
			.setStageFlags(vk::ShaderStageFlagBits::eAll)
		;
}

static auto create_bindings(const spirv_cross::Compiler& compiler, const std::vector<spirv_cross::Resource>& resources, ShaderType shader_type, vk::DescriptorType type) {
	auto bindings = std::unordered_map<u32, core::Vector<vk::DescriptorSetLayoutBinding>>();
	for(auto r : resources) {
		auto id = r.id;
		bindings[compiler.get_decoration(id, spv::DecorationDescriptorSet)] << create_binding(compiler.get_decoration(id, spv::DecorationBinding), shader_type, type);
	}
	return bindings;
}

static void fail_not_empty(const std::vector<spirv_cross::Resource>& res) {
	if(!res.empty()) {
		fatal("Unsupported resource type");
	}
}

static u32 component_size(spirv_cross::SPIRType type) {
	if(type.basetype == spirv_cross::SPIRType::Float) {
		return sizeof(float);
	}
	return fatal("Unknown attribute type");
}

static auto create_attribs(const spirv_cross::Compiler& compiler, const std::vector<spirv_cross::Resource>& resources) {
	core::Vector<ShaderModuleBase::Attribute> attribs;
	std::unordered_set<u32> locations;
	for(auto r : resources) {
		auto location = compiler.get_decoration(r.id, spv::DecorationLocation);
		auto type = compiler.get_type(r.type_id);
		attribs << ShaderModuleBase::Attribute{location, type.columns, type.vecsize, component_size(type)};

		for(auto i : core::range(location, location + type.columns)) {
			if(!locations.insert(i).second) {
				fatal("Duplicate or overlapping attributes location");
			}
		}
	}
	return attribs;
}

ShaderModuleBase::ShaderModuleBase(DevicePtr dptr, const SpirVData& data) : DeviceLinked(dptr), _module(create_shader_module(dptr, data)) {
	spirv_cross::Compiler compiler(std::vector<u32>(data.data(), data.data() + data.size() / 4));

	_type = module_type(compiler);

	auto resources = compiler.get_shader_resources();
	merge(_bindings, create_bindings(compiler, resources.uniform_buffers, _type, vk::DescriptorType::eUniformBuffer));
	merge(_bindings, create_bindings(compiler, resources.storage_buffers, _type, vk::DescriptorType::eStorageBuffer));
	merge(_bindings, create_bindings(compiler, resources.sampled_images, _type, vk::DescriptorType::eCombinedImageSampler));

	// these are attribs & other stages stuff
	/*fail_not_empty(resources.stage_inputs);
	fail_not_empty(resources.stage_outputs);
	fail_not_empty(resources.subpass_inputs);*/

	_attribs = create_attribs(compiler, resources.stage_inputs);

	fail_not_empty(resources.storage_images);
	fail_not_empty(resources.atomic_counters);
	fail_not_empty(resources.push_constant_buffers);
	fail_not_empty(resources.separate_images);
	fail_not_empty(resources.separate_samplers);
}

vk::ShaderModule ShaderModuleBase::vk_shader_module() const {
	return _module;
}

ShaderModuleBase::~ShaderModuleBase() {
	destroy(_module);
}

void ShaderModuleBase::swap(ShaderModuleBase& other) {
	DeviceLinked::swap(other);
	std::swap(_module, other._module);
}


}
