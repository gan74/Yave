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
#include "ShaderModule.h"

#include <yave/mesh/Vertex.h>
#include <yave/Device.h>

namespace yave {

MaterialCompiler::MaterialCompiler(DevicePtr dptr) : DeviceLinked(dptr), _ds_builder(dptr) {
}

ShaderModule MaterialCompiler::create_shader_module(const SpirVData& data) const {
	if(!data.size()) {
		return ShaderModule();
	}
	return ShaderModule(get_device(), get_device()->get_vk_device().createShaderModule(vk::ShaderModuleCreateInfo()
			.setCodeSize(data.size())
			.setPCode(data.data())
		));
}

GraphicPipeline MaterialCompiler::compile(const Material& material, const RenderPass& render_pass, Viewport view) const {
	auto geom_module = create_shader_module(material.get_geom_data());
	auto vert_module = create_shader_module(material.get_vert_data());
	auto frag_module = create_shader_module(material.get_frag_data());

	auto vert_stage_info = vk::PipelineShaderStageCreateInfo()
			.setModule(vert_module.get_vk_shader_module())
			.setStage(vk::ShaderStageFlagBits::eVertex)
			.setPName("main")
		;

	auto frag_stage_info = vk::PipelineShaderStageCreateInfo()
			.setModule(frag_module.get_vk_shader_module())
			.setStage(vk::ShaderStageFlagBits::eFragment)
			.setPName("main")
		;

	auto stage_create_infos = core::vector(vert_stage_info, frag_stage_info);

	if(geom_module.get_vk_shader_module()) {
		stage_create_infos << vk::PipelineShaderStageCreateInfo()
				.setModule(geom_module.get_vk_shader_module())
				.setStage(vk::ShaderStageFlagBits::eGeometry)
				.setPName("main")
			;
	}

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

	auto scissor = vk::Rect2D()
			.setExtent(vk::Extent2D(view.extent.x(), view.extent.y()))
			.setOffset({0, 0})
		;

	auto viewport_state = vk::PipelineViewportStateCreateInfo()
			.setViewportCount(1)
			.setPViewports(&viewport)
			.setScissorCount(1)
			.setPScissors(&scissor)
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

	auto color_blending = vk::PipelineColorBlendStateCreateInfo()
			.setLogicOpEnable(false)
			.setLogicOp(vk::LogicOp::eCopy)
			.setAttachmentCount(1)
			.setPAttachments(&color_blend_attachment)
			.setBlendConstants({0.0f, 0.0f, 0.0f, 0.0f})
		;

	auto depth_testing = vk::PipelineDepthStencilStateCreateInfo()
			.setDepthTestEnable(true)
			.setDepthWriteEnable(true)
			.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
		;

	auto d_set = _ds_builder.build(material);

	auto pipeline_layout = get_device()->get_vk_device().createPipelineLayout(vk::PipelineLayoutCreateInfo()
			.setSetLayoutCount(1)
			.setPSetLayouts(&d_set.layout)
		);

	auto pipeline = get_device()->get_vk_device().createGraphicsPipeline(VK_NULL_HANDLE, vk::GraphicsPipelineCreateInfo()
			.setStageCount(stage_create_infos.size())
			.setPStages(stage_create_infos.begin())
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

	return GraphicPipeline(get_device(), pipeline, pipeline_layout, d_set);
}

}
