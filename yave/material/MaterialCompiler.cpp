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
#include "MaterialCompiler.h"

#include <yave/shaders/ShaderModule.h>
#include <yave/mesh/Vertex.h>
#include <yave/Device.h>

#include <yave/shaders/ShaderProgram.h>

namespace yave {

MaterialCompiler::MaterialCompiler(DevicePtr dptr) : DeviceLinked(dptr) {
}

GraphicPipeline MaterialCompiler::compile(const Material& material, const RenderPass& render_pass, Viewport view) const {
	auto modules = core::vector(material.get_data()._vert, material.get_data()._frag);
	if(!material.get_data()._geom.is_empty()) {
		modules << material.get_data()._geom;
	}

	ShaderProgram program(material.get_device(), modules);
	auto pipeline_shader_stage = program.get_vk_pipeline_stage_info();

	auto binding_description = vk::VertexInputBindingDescription()
			.setBinding(0)
			.setStride(sizeof(Vertex))
			.setInputRate(vk::VertexInputRate::eVertex)
		;

	auto pos_attrib_description = vk::VertexInputAttributeDescription()
			.setBinding(0)
			.setLocation(0)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(offsetof(Vertex, position))
		;

	auto nor_attrib_description = vk::VertexInputAttributeDescription()
			.setBinding(0)
			.setLocation(1)
			.setFormat(vk::Format::eR32G32B32Sfloat)
			.setOffset(offsetof(Vertex, normal))
		;

	auto uv_attrib_description = vk::VertexInputAttributeDescription()
			.setBinding(0)
			.setLocation(2)
			.setFormat(vk::Format::eR32G32Sfloat)
			.setOffset(offsetof(Vertex, uv))
		;

	vk::VertexInputAttributeDescription attribs[] = {pos_attrib_description, nor_attrib_description, uv_attrib_description};

	auto vertex_input = vk::PipelineVertexInputStateCreateInfo()
			.setVertexAttributeDescriptionCount(3)
			.setPVertexAttributeDescriptions(attribs)
			.setVertexBindingDescriptionCount(1)
			.setPVertexBindingDescriptions(&binding_description)
		;

	auto input_assembly = vk::PipelineInputAssemblyStateCreateInfo()
			.setTopology(vk::PrimitiveTopology::eTriangleList)
			.setPrimitiveRestartEnable(false)
		;

	auto viewport = vk::Viewport()
			.setWidth(view.extent.x())
			.setHeight(view.extent.y())
			.setX(view.offset.x())
			.setY(view.offset.y())
			.setMinDepth(view.depth.x())
			.setMaxDepth(view.depth.y())
		;

	auto viewport_state = vk::PipelineViewportStateCreateInfo()
			.setViewportCount(1)
			.setPViewports(&viewport)
			.setScissorCount(1)
		;

	auto rasterizer = vk::PipelineRasterizationStateCreateInfo()
			.setCullMode(vk::CullModeFlagBits::eBack)
			//.setCullMode(vk::CullModeFlagBits::eNone)
			.setPolygonMode(vk::PolygonMode::eFill)
			.setLineWidth(1.0f)
			.setFrontFace(vk::FrontFace::eClockwise)
			.setDepthBiasEnable(false)
			.setDepthClampEnable(false)
		;

	auto multisampling = vk::PipelineMultisampleStateCreateInfo()
			.setSampleShadingEnable(false)
			.setRasterizationSamples(vk::SampleCountFlagBits::e1)
		;

	auto color_blend_attachment = vk::PipelineColorBlendAttachmentState()
			.setBlendEnable(false)
			.setColorWriteMask(vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eG | vk::ColorComponentFlagBits::eA)
		;

	auto att_blends = core::range(usize(0), render_pass.attachment_count()).map([=](usize) { return color_blend_attachment; }).collect<core::Vector>();

	auto color_blending = vk::PipelineColorBlendStateCreateInfo()
			.setLogicOpEnable(false)
			.setLogicOp(vk::LogicOp::eCopy)
			.setAttachmentCount(u32(att_blends.size()))
			.setPAttachments(att_blends.begin())
			.setBlendConstants({{0.0f, 0.0f, 0.0f, 0.0f}})
		;

	auto depth_testing = vk::PipelineDepthStencilStateCreateInfo()
			.setDepthTestEnable(true)
			.setDepthWriteEnable(true)
			.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
		;

	auto pipeline_layout = get_device()->get_vk_device().createPipelineLayout(vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(u32(program.get_descriptor_layouts().size()))
			.setPSetLayouts(program.get_descriptor_layouts().begin())
		);

	auto pipeline = get_device()->get_vk_device().createGraphicsPipeline(VK_NULL_HANDLE, vk::GraphicsPipelineCreateInfo()
			.setStageCount(u32(pipeline_shader_stage.size()))
			.setPStages(pipeline_shader_stage.begin())
			.setPVertexInputState(&vertex_input)
			.setPInputAssemblyState(&input_assembly)
			.setPViewportState(&viewport_state)
			.setPRasterizationState(&rasterizer)
			.setPMultisampleState(&multisampling)
			.setPColorBlendState(&color_blending)
			.setPDepthStencilState(&depth_testing)
			.setLayout(pipeline_layout)
			.setRenderPass(render_pass.get_vk_render_pass())
			.setSubpass(0)
			.setBasePipelineIndex(-1)
		);

	return GraphicPipeline(material, pipeline, pipeline_layout);
}

}
