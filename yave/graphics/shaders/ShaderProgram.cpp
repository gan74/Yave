/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include <yave/graphics/graphics.h>
#include <yave/graphics/descriptors/DescriptorSetAllocator.h>

#include <numeric>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <external/spirv_cross/spirv.hpp>
#include <external/spirv_cross/spirv_cross.hpp>
#include <external/spirv_cross/spirv_glsl.hpp>

namespace yave {

using Attribs = core::Vector<VkVertexInputAttributeDescription>;
using Bindings = core::Vector<VkVertexInputBindingDescription>;

template<typename T, typename S>
static void merge_bindings(T& into, const S& o) {
    into.insert(o.begin(), o.end());
}

template<typename T, typename S>
static void merge_push_constants(T& into, const S& o) {
    into.push_back(o.begin(), o.end());
}


static VkFormat vec_format(const ShaderModuleBase::Attribute& attr) {
    static_assert(VK_FORMAT_R32G32B32A32_SFLOAT == VK_FORMAT_R32G32B32A32_UINT + uenum(ShaderModuleBase::AttribType::Float));

    const usize type = usize(attr.type);
    switch(attr.vec_size) {
        case 1:
            return VkFormat(VK_FORMAT_R32_UINT + type);
        case 2:
            return VkFormat(VK_FORMAT_R32G32_UINT + type);
        case 3:
            return VkFormat(VK_FORMAT_R32G32B32_UINT + type);
        case 4:
            return VkFormat(VK_FORMAT_R32G32B32A32_UINT + type);

        default:
            break;
    }
    y_fatal("Unsupported vec format.");
}

static auto create_stage_info(core::Vector<VkPipelineShaderStageCreateInfo>& stages, const ShaderModuleBase& mod) {
    if(mod.vk_shader_module()) {
        VkPipelineShaderStageCreateInfo create_info = vk_struct();
        {
            create_info.module = mod.vk_shader_module();
            create_info.stage = VkShaderStageFlagBits(mod.type());
            create_info.pName = "main";
        }
        stages << create_info;
    }
}

// returns the NEXT binding index
static u32 create_vertex_attribs(u32 binding,
                                 VkVertexInputRate rate,
                                 core::Span<ShaderModuleBase::Attribute> vertex_attribs,
                                 Bindings& bindings,
                                 Attribs& attribs) {

    if(!vertex_attribs.is_empty()) {
        u32 offset = 0;
        for(const auto& attr : vertex_attribs) {
            const auto format = vec_format(attr);
            for(u32 i = 0; i != attr.columns; ++i) {
                attribs << VkVertexInputAttributeDescription{attr.location + i, binding, format, offset};
                offset += attr.vec_size * attr.component_size;
            }
        }
        bindings << VkVertexInputBindingDescription{binding, offset, rate};
        return binding + 1;
    }
    return binding;
}

// Takes a SORTED (by location) Attribute list
static void create_vertex_attribs(core::Span<ShaderModuleBase::Attribute> vertex_attribs,
                                  Bindings& bindings,
                                  Attribs& attribs) {

    y_debug_assert(std::is_sorted(vertex_attribs.begin(), vertex_attribs.end(), [](const auto& a, const auto& b) { return a.location < b.location; }));

    core::Vector<ShaderModuleBase::Attribute> v_attribs;
    core::Vector<ShaderModuleBase::Attribute> i_attribs;

    for(const auto& attr : vertex_attribs) {
        (attr.location < ShaderProgram::per_instance_location ? v_attribs : i_attribs) << attr;
    }

    u32 inst = create_vertex_attribs(0, VK_VERTEX_INPUT_RATE_VERTEX, v_attribs, bindings, attribs);

    for(const auto& attrib : i_attribs) {
        inst = create_vertex_attribs(inst, VK_VERTEX_INPUT_RATE_INSTANCE, {attrib}, bindings, attribs);
    }
}

static void validate_bindings(core::Span<VkDescriptorSetLayoutBinding> bindings) {
    u32 max = 0;
    for(u32 i = 0; i != bindings.size(); ++i) {
        max = std::max(max, bindings[i].binding + 1);
    }
    unused(max);
    y_debug_assert(max == bindings.size());
}

ShaderProgram::ShaderProgram(const FragmentShader& frag, const VertexShader& vert, const GeometryShader& geom) {
    {
        merge_bindings(_bindings, frag.bindings());
        merge_bindings(_bindings, vert.bindings());
        merge_bindings(_bindings, geom.bindings());

        create_stage_info(_stages, frag);
        create_stage_info(_stages, vert);
        create_stage_info(_stages, geom);

        merge_push_constants(_push_constants, frag.vk_push_constants());
        merge_push_constants(_push_constants, vert.vk_push_constants());
        merge_push_constants(_push_constants, geom.vk_push_constants());

        const u32 max_set = std::accumulate(_bindings.begin(), _bindings.end(), 0, [](u32 max, const auto& p) { return std::max(max, p.first); });

        if(!_bindings.is_empty()) {
            _layouts = core::Vector<VkDescriptorSetLayout>(max_set + 1, VkDescriptorSetLayout{});
            for(const auto& binding : _bindings) {
                validate_bindings(binding.second);
                _layouts[binding.first] = descriptor_set_allocator().descriptor_set_layout(binding.second).vk_descriptor_set_layout();
            }
        }
    }
    {
        auto vertex_attribs = vert.attributes();
        std::sort(vertex_attribs.begin(), vertex_attribs.end(), [](const auto& a, const auto& b) { return a.location < b.location; });
        create_vertex_attribs(vertex_attribs, _vertex.bindings, _vertex.attribs);
    }
    {
        _fragment_outputs = frag.stage_output();
        std::sort(_fragment_outputs.begin(), _fragment_outputs.end());
    }
}

core::Span<VkPipelineShaderStageCreateInfo> ShaderProgram::vk_pipeline_stage_info() const {
    return _stages;
}

core::Span<VkDescriptorSetLayout> ShaderProgram::vk_descriptor_layouts() const {
    return _layouts;
}

core::Span<VkVertexInputBindingDescription> ShaderProgram::vk_attribute_bindings() const {
    return _vertex.bindings;
}

core::Span<VkVertexInputAttributeDescription> ShaderProgram::vk_attributes_descriptions() const {
    return _vertex.attribs;
}

core::Span<VkPushConstantRange> ShaderProgram::vk_push_constants() const {
    return _push_constants;
}

core::Span<u32> ShaderProgram::fragment_outputs() const {
    return _fragment_outputs;
}

}

