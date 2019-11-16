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
	const GeometryShader geom = mat_data._geom.is_empty() ? GeometryShader() : GeometryShader(dptr, mat_data._geom);
	const ShaderProgram program(frag, vert, geom);

	core::Vector<vk::PipelineShaderStageCreateInfo> pipeline_shader_stages(program.vk_pipeline_stage_info());
	if(render_pass.is_depth_only()) {
		depth_only_stages(pipeline_shader_stages);
	}

	auto attribute_bindings = program.attribute_bindings();
	auto attribute_descriptions = program.attributes_descriptions();

	const auto vertex_input = vk::PipelineVertexInputStateCreateInfo()
			.setVertexAttributeDescriptionCount(attribute_descriptions.size())
			.setPVertexAttributeDescriptions(attribute_descriptions.begin())
			.setVertexBindingDescriptionCount(attribute_bindings.size())
			.setPVertexBindingDescriptions(attribute_bindings.begin())
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
			.setCullMode(mat_data._cull ? vk::CullModeFlagBits::eBack : vk::CullModeFlagBits::eNone)
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

	const auto color_blend_attachment = vk::PipelineColorBlendAttachmentState()
			.setBlendEnable(mat_data._blend)
			.setColorBlendOp(vk::BlendOp::eAdd)
			.setAlphaBlendOp(vk::BlendOp::eAdd)
			.setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
			.setDstAlphaBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
			.setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
			.setSrcAlphaBlendFactor(vk::BlendFactor::eSrcAlpha)
			.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eA)
		;

	const auto att_blends = core::Vector<vk::PipelineColorBlendAttachmentState>(render_pass.attachment_count(), color_blend_attachment);

	const auto color_blending = vk::PipelineColorBlendStateCreateInfo()
			.setLogicOpEnable(false)
			.setLogicOp(vk::LogicOp::eCopy)
			.setAttachmentCount(u32(att_blends.size()))
			.setPAttachments(att_blends.begin())
			.setBlendConstants({{0.0f, 0.0f, 0.0f, 0.0f}})
		;

	const auto depth_testing = vk::PipelineDepthStencilStateCreateInfo()
			.setDepthTestEnable(mat_data._depth_tested != DepthTest::None)
			.setDepthWriteEnable(true)
			.setDepthCompareOp(mat_data._depth_tested == DepthTest::Reversed
							   ? vk::CompareOp::eLessOrEqual
							   : vk::CompareOp::eGreaterOrEqual) // reversed Z
		;

	const auto pipeline_layout = device()->vk_device().createPipelineLayout(vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(u32(program.descriptor_layouts().size()))
			.setPSetLayouts(program.descriptor_layouts().begin())
			.setPushConstantRangeCount(u32(program.push_constants().size()))
			.setPPushConstantRanges(program.push_constants().begin())
		);

	const std::array<vk::DynamicState, 2> dynamics = {{vk::DynamicState::eViewport, vk::DynamicState::eScissor}};
	const auto dynamic_states = vk::PipelineDynamicStateCreateInfo()
			.setDynamicStateCount(dynamics.size())
			.setPDynamicStates(dynamics.begin())
		;

	const auto pipeline = device()->vk_device().createGraphicsPipeline(vk::PipelineCache(), vk::GraphicsPipelineCreateInfo()
			.setStageCount(u32(pipeline_shader_stages.size()))
			.setPStages(pipeline_shader_stages.begin())
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
