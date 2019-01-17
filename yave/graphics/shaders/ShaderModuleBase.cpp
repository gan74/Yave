/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

#include "ShaderModuleBase.h"

#include <yave/device/Device.h>

#include <spirv_cross/spirv.hpp>
#include <spirv_cross/spirv_cross.hpp>

namespace yave {

template<typename M>
static void merge(M& into, const M& o) {
	for(const auto& p : o) {
		into[p.first].push_back(p.second.begin(), p.second.end());
	}
}

static vk::ShaderModule create_shader_module(DevicePtr dptr, const SpirVData& data) {
	if(data.is_empty()) {
		return vk::ShaderModule();
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
	return y_fatal("Unknown shader execution model.");
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
	for(const auto& r : resources) {
		auto id = r.id;
		bindings[compiler.get_decoration(id, spv::DecorationDescriptorSet)] << create_binding(compiler.get_decoration(id, spv::DecorationBinding), shader_type, type);
	}
	return bindings;
}

static void fail_not_empty(const std::vector<spirv_cross::Resource>& res) {
	if(!res.empty()) {
		y_fatal("Unsupported resource type.");
	}
}

static u32 component_size(spirv_cross::SPIRType::BaseType type) {
	switch(type) {
		case spirv_cross::SPIRType::Float:
		case spirv_cross::SPIRType::Int:
		case spirv_cross::SPIRType::UInt:
			return 4;

		case spirv_cross::SPIRType::Char:
			return 1;

		default:
			break;
	}
	return y_fatal("Unsupported attribute type.");
}

static ShaderModuleBase::AttribType component_type(spirv_cross::SPIRType::BaseType type) {
	switch(type) {
		case spirv_cross::SPIRType::Float:
			return ShaderModuleBase::AttribType::Float;

		case spirv_cross::SPIRType::Int:
			return ShaderModuleBase::AttribType::Int;

		case spirv_cross::SPIRType::UInt:
			return ShaderModuleBase::AttribType::Uint;

		case spirv_cross::SPIRType::Char:
			return ShaderModuleBase::AttribType::Char;

		default:
			break;
	}
	return y_fatal("Unsupported attribute type.");
}

static auto create_push_constants(const spirv_cross::Compiler& compiler, const std::vector<spirv_cross::Resource>& resources, ShaderType shader_type) {
	if(resources.size() > 1) {
		y_fatal("Too many push constants.");
	}

	core::Vector<vk::PushConstantRange> push_constants;
	for(const auto& r : resources) {
		const auto& type = compiler.get_type(r.type_id);
		push_constants << vk::PushConstantRange(vk::ShaderStageFlagBits(shader_type), 0, compiler.get_declared_struct_size(type));
	}
	return push_constants;
}


static auto create_attribs(const spirv_cross::Compiler& compiler, const std::vector<spirv_cross::Resource>& resources) {
	core::Vector<ShaderModuleBase::Attribute> attribs;
	std::unordered_set<u32> locations;
	for(const auto& r : resources) {
		auto location = compiler.get_decoration(r.id, spv::DecorationLocation);
		const auto& type = compiler.get_type(r.type_id);

		attribs << ShaderModuleBase::Attribute{location, type.columns, type.vecsize, component_size(type.basetype), component_type(type.basetype)};

		for(usize i = location; i != location + type.columns; ++i) {
			if(!locations.insert(i).second) {
				y_fatal("Duplicate or overlapping attribute locations.");
			}
		}
	}
	return attribs;
}


ShaderType ShaderModuleBase::shader_type(const SpirVData& data) {
	spirv_cross::Compiler compiler(std::vector<u32>(data.data(), data.data() + data.size() / 4));
	return module_type(compiler);
}

ShaderModuleBase::ShaderModuleBase(DevicePtr dptr, const SpirVData& data) : DeviceLinked(dptr), _module(create_shader_module(dptr, data)) {
	spirv_cross::Compiler compiler(std::vector<u32>(data.data(), data.data() + data.size() / 4));

	_type = module_type(compiler);

	auto resources = compiler.get_shader_resources();
	merge(_bindings, create_bindings(compiler, resources.uniform_buffers, _type, vk::DescriptorType::eUniformBuffer));
	merge(_bindings, create_bindings(compiler, resources.storage_buffers, _type, vk::DescriptorType::eStorageBuffer));
	merge(_bindings, create_bindings(compiler, resources.sampled_images, _type, vk::DescriptorType::eCombinedImageSampler));
	merge(_bindings, create_bindings(compiler, resources.storage_images, _type, vk::DescriptorType::eStorageImage));

	_push_constants = create_push_constants(compiler, resources.push_constant_buffers, _type);

	_attribs = create_attribs(compiler, resources.stage_inputs);

	// these are attribs & other stages stuff
	fail_not_empty(resources.atomic_counters);
	fail_not_empty(resources.separate_images);
	fail_not_empty(resources.separate_samplers);

	u32 spec_offset = 0;
	for(const auto& cst : compiler.get_specialization_constants()) {
		const auto& type = compiler.get_type(compiler.get_constant(cst.id).constant_type);
		u32 size = type.width / 8;
		_spec_constants << vk::SpecializationMapEntry(cst.constant_id, spec_offset, size);
		spec_offset += size;
	}

	for(usize i = 0; i != 3; ++i) {
		_local_size[i] = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, i);
	}
}

ShaderModuleBase::~ShaderModuleBase() {
	destroy(_module);
}

}
