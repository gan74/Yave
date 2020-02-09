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
#include "MaterialCompiler.h"

#include <yave/graphics/shaders/ShaderProgram.h>
#include <yave/meshes/Vertex.h>
#include <yave/device/Device.h>

#include <y/core/Chrono.h>

namespace yave {

static void depth_only_stages(core::Vector<vk::PipelineShaderStageCreateInfo>& stages) {
	const auto is_frag_stage = [](const vk::PipelineShaderStageCreateInfo& s) { return s.stage == vk::ShaderStageFlagBits::eFragment; };
	if(const auto it = std::find_if(stages.begin(), stages.end(), is_frag_stage); it != stages.end()) {
		stages.erase_unordered(it);
	}
}

static vk::BlendFactor src_blend_factor(BlendMode mode) {
	switch(mode) {
		case BlendMode::SrcAlpha:
			return vk::BlendFactor::eSrcAlpha;

		default:
			return vk::BlendFactor::eOne;
	}
}

static vk::BlendFactor dst_blend_factor(BlendMode mode) {
	switch(mode) {
		case BlendMode::SrcAlpha:
			return vk::BlendFactor::eOneMinusSrcAlpha;

		default:
			return vk::BlendFactor::eOne;
	}
}

static vk::CullModeFlags cull_mode(CullMode mode) {
	switch(mode) {
		case CullMode::None:
			return vk::CullModeFlagBits::eNone;

		case CullMode::Front:
			return vk::CullModeFlagBits::eFront;

		default:
			return vk::CullModeFlagBits::eBack;
	}
}

static vk::CompareOp depth_mode(DepthTestMode mode) {
	switch(mode) {
		case DepthTestMode::Standard:
			return vk::CompareOp::eGreaterOrEqual;

		case DepthTestMode::Reversed:
			return vk::CompareOp::eLessOrEqual;

		default: // None
			return vk::CompareOp::eAlways;
	}
}

static GeometryShader create_geometry_shader(DevicePtr dptr, const SpirVData& geom) {
	if(geom.is_empty()) {
		return GeometryShader();
	}
	return GeometryShader(dptr, geom);
}


MaterialCompiler::MaterialCompiler(DevicePtr dptr) : DeviceLinked(dptr) {
}

GraphicPipeline MaterialCompiler::compile(const MaterialTemplate* material, const RenderPass& render_pass) const {
	y_profile();
	core::DebugTimer _("MaterialCompiler::compile", core::Duration::milliseconds(2));
	Y_TODO(move program creation programs can be reused)

	DevicePtr dptr = material->device();
	const auto& mat_data = material->data();

	const FragmentShader frag = FragmentShader(dptr, mat_data._frag);
	const VertexShader vert = VertexShader(dptr, mat_data._vert);
	const GeometryShader geom = create_geometry_shader(dptr, mat_data._geom);
	const ShaderProgram program(frag, vert, geom);

	core::Vector<vk::PipelineShaderStageCreateInfo> pipeline_shader_stages(program.vk_pipeline_stage_info());
	if(render_pass.is_depth_only()) {
		depth_only_stages(pipeline_shader_stages);
	}

	auto attribute_bindings = program.attribute_bindings();
	auto attribute_descriptions = program.attributes_descriptions();

	const auto vertex_input = vk::PipelineVertexInputStateCreateInfo()
			.setVertexAttributeDescriptionCount(attribute_descriptions.size())
			.setPVertexAttributeDescriptions(attribute_descriptions.data())
			.setVertexBindingDescriptionCount(attribute_bindings.size())
			.setPVertexBindingDescriptions(attribute_bindings.data())
		;

	const auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology(vk::PrimitiveTopology(mat_data._primitive_type))
			.setPrimitiveRestartEnable(false)
		;

	auto viewport = vk::Viewport();
	auto scissor = vk::Rect2D();

	const auto viewport_state = vk::PipelineViewportStateCreateInfo()
			.setViewportCount(1)
			.setPViewports(&viewport)
			.setScissorCount(1)
			.setPScissors(&scissor)
		;

	const auto rasterizer = vk::PipelineRasterizationStateCreateInfo()
			.setCullMode(cull_mode(mat_data._cull_mode))
			//.setCullMode(vk::CullModeFlagBits::eNone)
			.setPolygonMode(vk::PolygonMode::eFill)
			.setLineWidth(1.0f)
			.setFrontFace(vk::FrontFace::eCounterClockwise)
			.setDepthBiasEnable(false)
			.setDepthClampEnable(false)
		;

	const auto multisampling = vk::PipelineMultisampleStateCreateInfo()
			.setSampleShadingEnable(false)
			.setRasterizationSamples(vk::SampleCountFlagBits::e1)
		;

	const auto src_blend = src_blend_factor(mat_data._blend_mode);
	const auto dst_blend = dst_blend_factor(mat_data._blend_mode);
	const auto color_blend_attachment = vk::PipelineColorBlendAttachmentState()
			.setBlendEnable(mat_data._blend_mode != BlendMode::None)
			.setColorBlendOp(vk::BlendOp::eAdd)
			.setAlphaBlendOp(vk::BlendOp::eAdd)
			.setDstColorBlendFactor(dst_blend)
			.setDstAlphaBlendFactor(dst_blend)
			.setSrcColorBlendFactor(src_blend)
			.setSrcAlphaBlendFactor(src_blend)
			.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eA)
		;

	const auto att_blends = core::Vector<vk::PipelineColorBlendAttachmentState>(render_pass.attachment_count(), color_blend_attachment);

	const auto color_blending = vk::PipelineColorBlendStateCreateInfo()
			.setLogicOpEnable(false)
			.setLogicOp(vk::LogicOp::eCopy)
			.setAttachmentCount(u32(att_blends.size()))
			.setPAttachments(att_blends.data())
			.setBlendConstants({{0.0f, 0.0f, 0.0f, 0.0f}})
		;

	const auto depth_testing = vk::PipelineDepthStencilStateCreateInfo()
			.setDepthTestEnable(mat_data._depth_mode != DepthTestMode::None || mat_data._depth_write)
			.setDepthWriteEnable(mat_data._depth_write)
			.setDepthCompareOp(depth_mode(mat_data._depth_mode))
		;

	const auto pipeline_layout = device()->vk_device().createPipelineLayout(vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(u32(program.descriptor_layouts().size()))
			.setPSetLayouts(program.descriptor_layouts().data())
			.setPushConstantRangeCount(u32(program.push_constants().size()))
			.setPPushConstantRanges(program.push_constants().data())
		);

	const std::array<vk::DynamicState, 2> dynamics = {{vk::DynamicState::eViewport, vk::DynamicState::eScissor}};
	const auto dynamic_states = vk::PipelineDynamicStateCreateInfo()
			.setDynamicStateCount(dynamics.size())
			.setPDynamicStates(dynamics.data())
		;

	const auto pipeline = device()->vk_device().createGraphicsPipeline(vk::PipelineCache(), vk::GraphicsPipelineCreateInfo()
			.setStageCount(u32(pipeline_shader_stages.size()))
			.setPStages(pipeline_shader_stages.data())
			.setPDynamicState(&dynamic_states)
			.setPVertexInputState(&vertex_input)
			.setPInputAssemblyState(&input_assembly)
			.setPRasterizationState(&rasterizer)
			.setPViewportState(&viewport_state)
			.setPMultisampleState(&multisampling)
			.setPColorBlendState(&color_blending)
			.setPDepthStencilState(&depth_testing)
			.setLayout(pipeline_layout)
			.setRenderPass(render_pass.vk_render_pass())
			.setSubpass(0)
			.setBasePipelineIndex(-1)
		);

	return GraphicPipeline(material, pipeline, pipeline_layout);
}

}
