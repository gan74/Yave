/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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

static core::ArrayView<vk::PipelineShaderStageCreateInfo> depth_only_stages(core::ArrayView<vk::PipelineShaderStageCreateInfo> stages) {
	auto is_vertex_stage = [](const vk::PipelineShaderStageCreateInfo& s) { return s.stage == vk::ShaderStageFlagBits::eVertex; };
	y_debug_assert(std::count_if(stages.begin(), stages.end(), is_vertex_stage) == 1);
	return *std::find_if(stages.begin(), stages.end(), is_vertex_stage);
}

MaterialCompiler::MaterialCompiler(DevicePtr dptr) : DeviceLinked(dptr) {
}

GraphicPipeline MaterialCompiler::compile(const Material* material, const RenderPass& render_pass) const {
	Y_LOG_PERF("material");
	core::DebugTimer _("MaterialCompiler::compile", core::Duration::milliseconds(2));
#warning move program creation

	DevicePtr dptr = material->device();
	const auto& mat_data = material->data();

	FragmentShader frag = FragmentShader(dptr, mat_data._frag);
	VertexShader vert = VertexShader(dptr, mat_data._vert);
	GeometryShader geom = mat_data._geom.is_empty() ? GeometryShader() : GeometryShader(dptr, mat_data._geom);
	ShaderProgram program(frag, vert, geom);

	auto pipeline_shader_stage = program.vk_pipeline_stage_info();
	if(render_pass.is_depth_only()) {
		pipeline_shader_stage = depth_only_stages(pipeline_shader_stage);
	}

	auto attribute_bindings = program.attribute_bindings();
	auto attribute_descriptions = program.attributes_descriptions();

	auto vertex_input = vk::PipelineVertexInputStateCreateInfo()
			.setVertexAttributeDescriptionCount(attribute_descriptions.size())
			.setPVertexAttributeDescriptions(attribute_descriptions.begin())
			.setVertexBindingDescriptionCount(attribute_bindings.size())
			.setPVertexBindingDescriptions(attribute_bindings.begin())
		;

	auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology(vk::PrimitiveTopology(mat_data._primitive_type))
			.setPrimitiveRestartEnable(false)
		;

	auto viewport = vk::Viewport();
	auto scissor = vk::Rect2D();

	auto viewport_state = vk::PipelineViewportStateCreateInfo()
			.setViewportCount(1)
			.setPViewports(&viewport)
			.setScissorCount(1)
			.setPScissors(&scissor)
		;

	auto rasterizer = vk::PipelineRasterizationStateCreateInfo()
			.setCullMode(mat_data._cull ? vk::CullModeFlagBits::eBack : vk::CullModeFlagBits::eNone)
			//.setCullMode(vk::CullModeFlagBits::eNone)
			.setPolygonMode(vk::PolygonMode::eFill)
			.setLineWidth(1.0f)
			.setFrontFace(vk::FrontFace::eCounterClockwise)
			.setDepthBiasEnable(false)
			.setDepthClampEnable(false)
		;

	auto multisampling = vk::PipelineMultisampleStateCreateInfo()
			.setSampleShadingEnable(false)
			.setRasterizationSamples(vk::SampleCountFlagBits::e1)
		;

	auto color_blend_attachment = vk::PipelineColorBlendAttachmentState()
			.setBlendEnable(mat_data._blend)
			.setColorBlendOp(vk::BlendOp::eAdd)
			.setAlphaBlendOp(vk::BlendOp::eAdd)
			.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
			.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
			.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
			.setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
			.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eA)
		;

	auto att_blends = core::Vector<vk::PipelineColorBlendAttachmentState>(render_pass.attachment_count(), color_blend_attachment);

	auto color_blending = vk::PipelineColorBlendStateCreateInfo()
			.setLogicOpEnable(false)
			.setLogicOp(vk::LogicOp::eCopy)
			.setAttachmentCount(u32(att_blends.size()))
			.setPAttachments(att_blends.begin())
			.setBlendConstants({{0.0f, 0.0f, 0.0f, 0.0f}})
		;

	auto depth_testing = vk::PipelineDepthStencilStateCreateInfo()
			.setDepthTestEnable(mat_data._depth_tested)
			.setDepthWriteEnable(true)
			.setDepthCompareOp(vk::CompareOp::eGreaterOrEqual) // reversed Z
		;

	auto pipeline_layout = device()->vk_device().createPipelineLayout(vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(u32(program.descriptor_layouts().size()))
			.setPSetLayouts(program.descriptor_layouts().begin())
			.setPushConstantRangeCount(u32(program.push_constants().size()))
			.setPPushConstantRanges(program.push_constants().begin())
		);

	std::array<vk::DynamicState, 2> dynamics = {{vk::DynamicState::eViewport, vk::DynamicState::eScissor}};
	auto dynamic_states = vk::PipelineDynamicStateCreateInfo()
			.setDynamicStateCount(dynamics.size())
			.setPDynamicStates(dynamics.begin())
		;

	auto pipeline = device()->vk_device().createGraphicsPipeline(vk::PipelineCache(), vk::GraphicsPipelineCreateInfo()
			.setStageCount(u32(pipeline_shader_stage.size()))
			.setPStages(pipeline_shader_stage.begin())
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
