/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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
#include "MaterialTemplate.h"

#include <yave/graphics/framebuffer/RenderPass.h>
#include <yave/graphics/shaders/ShaderProgram.h>
#include <yave/meshes/Vertex.h>
#include <yave/graphics/graphics.h>

#include <y/core/ScratchPad.h>
#include <y/core/Chrono.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

static void keep_depth_only_stages(core::ScratchVector<VkPipelineShaderStageCreateInfo>& stages) {
    const auto is_frag_stage = [](const VkPipelineShaderStageCreateInfo& s) { return s.stage == VK_SHADER_STAGE_FRAGMENT_BIT; };
    if(const auto it = std::find_if(stages.begin(), stages.end(), is_frag_stage); it != stages.end()) {
        stages.erase_unordered(it);
    }
}

static VkBlendFactor src_blend_factor(BlendMode mode) {
    switch(mode) {
        case BlendMode::SrcAlpha:
            return VK_BLEND_FACTOR_SRC_ALPHA;

        default:
            return VK_BLEND_FACTOR_ONE;
    }
}

static VkBlendFactor dst_blend_factor(BlendMode mode) {
    switch(mode) {
        case BlendMode::SrcAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

        default:
            return VK_BLEND_FACTOR_ONE;
    }
}

static VkCullModeFlags cull_mode(CullMode mode) {
    switch(mode) {
        case CullMode::None:
            return VK_CULL_MODE_NONE;

        case CullMode::Front:
            return VK_CULL_MODE_FRONT_BIT;

        default:
            return VK_CULL_MODE_BACK_BIT;
    }
}

static VkCompareOp depth_mode(DepthTestMode mode) {
    switch(mode) {
        case DepthTestMode::Standard:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;

        case DepthTestMode::Reversed:
            return VK_COMPARE_OP_LESS_OR_EQUAL;

        default: // None
            return VK_COMPARE_OP_ALWAYS;
    }
}

static GeometryShader create_geometry_shader(const SpirVData& geom) {
    if(geom.is_empty()) {
        return GeometryShader();
    }
    return GeometryShader(geom);
}



GraphicPipeline MaterialCompiler::compile(const MaterialTemplate* material, const RenderPass& render_pass) {
    y_profile();

    core::DebugTimer _("MaterialCompiler::compile", core::Duration::milliseconds(2));
    Y_TODO(move program creation programs can be reused)

    const auto& mat_data = material->data();

    const FragmentShader frag = FragmentShader(mat_data._frag);
    const VertexShader vert = VertexShader(mat_data._vert);
    const GeometryShader geom = create_geometry_shader(mat_data._geom);
    const ShaderProgram program(frag, vert, geom);

    core::ScratchVector<VkPipelineShaderStageCreateInfo> pipeline_shader_stages(program.vk_pipeline_stage_info());
    if(render_pass.is_depth_only()) {
        keep_depth_only_stages(pipeline_shader_stages);
    }

    const auto attribute_bindings = program.vk_attribute_bindings();
    const auto attribute_descriptions = program.vk_attributes_descriptions();

    VkPipelineVertexInputStateCreateInfo vertex_input = vk_struct();
    {
        vertex_input.vertexAttributeDescriptionCount = u32(attribute_descriptions.size());
        vertex_input.pVertexAttributeDescriptions = attribute_descriptions.data();
        vertex_input.vertexBindingDescriptionCount = u32(attribute_bindings.size());
        vertex_input.pVertexBindingDescriptions = attribute_bindings.data();
    }

    VkPipelineInputAssemblyStateCreateInfo input_assembly = vk_struct();
    {
        input_assembly.topology = VkPrimitiveTopology(mat_data._primitive_type);
        input_assembly.primitiveRestartEnable = false;
    }

    const VkViewport viewport = {};
    const VkRect2D scissor = {};
    VkPipelineViewportStateCreateInfo viewport_state = vk_struct();
    {
        viewport_state.viewportCount = 1;
        viewport_state.scissorCount = 1;
        viewport_state.pViewports = &viewport;
        viewport_state.pScissors = &scissor;
    }

    VkPipelineRasterizationStateCreateInfo rasterizer = vk_struct();
    {
        rasterizer.cullMode = cull_mode(mat_data._cull_mode);
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = false;
        rasterizer.depthClampEnable = false;
    }

    VkPipelineMultisampleStateCreateInfo multisampling = vk_struct();
    {
        multisampling.sampleShadingEnable = false;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    }

    VkPipelineColorBlendAttachmentState color_blend_attachment = {};
    {
        const VkBlendFactor src_blend = src_blend_factor(mat_data._blend_mode);
        const VkBlendFactor dst_blend = dst_blend_factor(mat_data._blend_mode);

        color_blend_attachment.blendEnable = mat_data._blend_mode != BlendMode::None;
        color_blend_attachment.colorBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment.alphaBlendOp = VK_BLEND_OP_ADD;
        color_blend_attachment.dstColorBlendFactor = dst_blend;
        color_blend_attachment.dstAlphaBlendFactor = dst_blend;
        color_blend_attachment.srcColorBlendFactor = src_blend;
        color_blend_attachment.srcAlphaBlendFactor = src_blend;
    }

    auto att_blends = core::Vector<VkPipelineColorBlendAttachmentState>(render_pass.attachment_count(), color_blend_attachment);
    {
        usize index = 0;
        const auto& outputs = program.fragment_outputs();
        for(u32 i = 0; i != att_blends.size(); ++i) {
            if(index < outputs.size() && outputs[index] == i) {
                att_blends[i].colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
                ++index;
            }
        }
    }


    VkPipelineColorBlendStateCreateInfo color_blending = vk_struct();
    {
        color_blending.logicOpEnable = false;
        color_blending.logicOp = VK_LOGIC_OP_COPY;
        color_blending.attachmentCount = u32(att_blends.size());
        color_blending.pAttachments = att_blends.data();
        for(usize i = 0; i != 4; ++i) {
            color_blending.blendConstants[i] = 0.0f;
        }
    }

    VkPipelineDepthStencilStateCreateInfo depth_testing = vk_struct();
    {
        depth_testing.depthTestEnable = mat_data._depth_mode != DepthTestMode::None || mat_data._depth_write;
        depth_testing.depthWriteEnable = mat_data._depth_write;
        depth_testing.depthCompareOp = depth_mode(mat_data._depth_mode);
    }

    VkHandle<VkPipelineLayout> pipeline_layout;
    {
        VkPipelineLayoutCreateInfo create_info = vk_struct();
        create_info.setLayoutCount = u32(program.vk_descriptor_layouts().size());
        create_info.pSetLayouts = program.vk_descriptor_layouts().data();
        vk_check(vkCreatePipelineLayout(vk_device(), &create_info, vk_allocation_callbacks(), pipeline_layout.get_ptr_for_init()));
    }

    const std::array dynamics = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_states = vk_struct();
    {
        dynamic_states.dynamicStateCount = u32(dynamics.size());
        dynamic_states.pDynamicStates = dynamics.data();
    }

    VkGraphicsPipelineCreateInfo create_info = vk_struct();
    {
        create_info.stageCount = u32(pipeline_shader_stages.size());
        create_info.pStages = pipeline_shader_stages.data();
        create_info.pDynamicState = &dynamic_states;
        create_info.pVertexInputState = &vertex_input;
        create_info.pInputAssemblyState = &input_assembly;
        create_info.pRasterizationState = &rasterizer;
        create_info.pViewportState = &viewport_state;
        create_info.pMultisampleState = &multisampling;
        create_info.pColorBlendState = &color_blending;
        create_info.pDepthStencilState = &depth_testing;
        create_info.layout = pipeline_layout;
        create_info.renderPass = render_pass.vk_render_pass();
        create_info.subpass = 0;
        create_info.basePipelineIndex = -1;
    }

    VkHandle<VkPipeline> pipeline;
    vk_check(vkCreateGraphicsPipelines(vk_device(), {}, 1, &create_info, vk_allocation_callbacks(), pipeline.get_ptr_for_init()));
    return GraphicPipeline(std::move(pipeline), std::move(pipeline_layout));
}

}

