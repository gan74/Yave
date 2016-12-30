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

#include "ShaderProgram.h"
#include <yave/Device.h>

#include <unordered_map>
#include <numeric>

#include <spirv_cross/spirv.hpp>
#include <spirv_cross/spirv_cross.hpp>
#include <spirv_cross/spirv_glsl.hpp>

namespace yave {

using Attribs = core::Vector<vk::VertexInputAttributeDescription>;
using Bindings = core::Vector<vk::VertexInputBindingDescription>;

template<typename T>
static void merge(T& into, const T& o) {
	into.insert(o.begin(), o.end());
}

static vk::Format vec_format(ShaderModuleBase::Attribute attr) {
	switch(attr.vec_size) {
		case 1:
			return vk::Format::eR32Sfloat;
		case 2:
			return vk::Format::eR32G32Sfloat;
		case 3:
			return vk::Format::eR32G32B32Sfloat;
		case 4:
			return vk::Format::eR32G32B32A32Sfloat;

		default:
			break;
	}
	return fatal("Unsupported format");
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

static void create_vertex_attribs(u32 binding,
								  vk::VertexInputRate rate,
								  const core::Vector<ShaderModuleBase::Attribute>& vertex_attribs,
								  Bindings& bindings,
								  Attribs& attribs) {

	if(!vertex_attribs.is_empty()) {
		u32 offset = 0;
		for(const auto& attr : vertex_attribs) {
			auto format = vec_format(attr);
			for(u32 i = 0; i != attr.columns; i++) {
				attribs << vk::VertexInputAttributeDescription()
						.setBinding(binding)
						.setLocation(attr.location + i)
						.setFormat(format)
						.setOffset(offset)
					;
				offset += attr.vec_size * attr.component_size;
			}
		}
		bindings << vk::VertexInputBindingDescription()
				.setBinding(binding)
				.setStride(offset)
				.setInputRate(rate)
			;
	}
}

// Takes a SORTED (by location) Attribute list
static void create_vertex_attribs(const core::Vector<ShaderModuleBase::Attribute>& vertex_attribs,
								  Bindings& bindings,
								  Attribs& attribs) {

	core::Vector<ShaderModuleBase::Attribute> v_attribs;
	core::Vector<ShaderModuleBase::Attribute> i_attribs;

	for(const auto& attr : vertex_attribs) {
		(attr.location < ShaderProgram::PerInstanceLocation ? v_attribs : i_attribs) << attr;
	}
	create_vertex_attribs(0, vk::VertexInputRate::eVertex, v_attribs, bindings, attribs);
	create_vertex_attribs(1, vk::VertexInputRate::eInstance, i_attribs, bindings, attribs);
}



ShaderProgram::ShaderProgram(const FragmentShader& frag, const VertexShader& vert, const GeometryShader& geom) : DeviceLinked(common_device(frag, vert, geom)) {
	{
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
	}
	{
		auto vertex_attribs = vert.attributes();
		std::sort(vertex_attribs.begin(), vertex_attribs.end(), [](const auto& a, const auto& b) { return a.location < b.location; });
		create_vertex_attribs(vertex_attribs, _vertex.bindings, _vertex.attribs);
	}
}

core::Vector<vk::PipelineShaderStageCreateInfo> ShaderProgram::vk_pipeline_stage_info() const {
	return _stages;
}

const core::Vector<vk::DescriptorSetLayout>& ShaderProgram::descriptor_layouts() const {
	return _layouts;
}

}
