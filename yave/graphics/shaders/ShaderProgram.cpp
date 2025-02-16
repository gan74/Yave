/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#include <yave/graphics/device/deviceutils.h>
#include <yave/graphics/device/DeviceProperties.h>
#include <yave/graphics/descriptors/DescriptorSetAllocator.h>
#include <yave/graphics/images/TextureLibrary.h>


#include <y/core/ScratchPad.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

#include <numeric>

namespace yave {

using Attribs = core::Vector<VkVertexInputAttributeDescription>;
using Bindings = core::Vector<VkVertexInputBindingDescription>;

template<typename... Ts>
static auto make_shader_array(const Ts&... args) {
    return std::array<const ShaderModuleBase*, sizeof...(args)> { &args... };
}

template<typename T, typename S>
static void merge_bindings(T& into, const S& other) {
    for(const auto& [k, v] : other) {
        into[k].push_back(v.begin(), v.end());
    }
}

template<typename T, typename S>
static void merge(T& into, const S& other) {
    for(const auto& e : other) {
        into.push_back(e);
    }
}

static void create_stage_info(core::Vector<VkPipelineShaderStageCreateInfo>& stages, const ShaderModuleBase& module) {
    if(!module.vk_shader_module()) {
        return;
    }

    VkPipelineShaderStageCreateInfo create_info = vk_struct();
    {
        create_info.module = module.vk_shader_module();
        create_info.stage = VkShaderStageFlagBits(module.type());
        create_info.pName = module.entry_point().data();
    }
    stages << create_info;
}

// Takes a SORTED (by location) Attribute list
static void create_vertex_attribs(core::Span<ShaderModuleBase::Attribute> vertex_attribs,
                                  Bindings& bindings,
                                  Attribs& attribs) {

    y_debug_assert(std::is_sorted(vertex_attribs.begin(), vertex_attribs.end(), [](const auto& a, const auto& b) { return a.location < b.location; }));

    u32 offset = 0;
    usize per_instance_offset = 0;
    for(const auto& attr : vertex_attribs) {
        const bool is_packed = attr.is_packed && !attribs.is_empty();
        const bool per_instance = attr.location >= ShaderProgram::per_instance_location;
        const VkVertexInputRate input_rate = per_instance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX;

        if(per_instance && !per_instance_offset) {
            per_instance_offset = ShaderProgram::per_instance_binding - bindings.size();
        }

        u32 binding = u32(bindings.size() + per_instance_offset);
        if(!is_packed) {
            offset = 0;
        } else {
            --binding;
        }

        const u32 component_size = u32(ImageFormat(attr.format).bit_per_pixel() / 8);
        for(u32 i = 0; i != attr.component_count; ++i) {
            attribs << VkVertexInputAttributeDescription{attr.location + i, binding, attr.format, offset};
            offset += component_size;
        }

        if(is_packed) {
            bindings.last().stride = offset;
        } else {
            bindings << VkVertexInputBindingDescription{binding, offset, input_rate};
        }
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







ShaderProgramBase::ShaderProgramBase(core::Span<const ShaderModuleBase*> shaders) {
    y_profile();

    for(const ShaderModuleBase* shader : shaders) {
        merge_bindings(_bindings, shader->bindings());
    }

    for(const ShaderModuleBase* shader : shaders) {
        create_stage_info(_stages, *shader);
    }

    for(auto& [set, bindings] : _bindings) {
        std::sort(bindings.begin(), bindings.end(), [](const auto& a, const auto& b) { return a.binding < b.binding; });
        const usize new_size = std::unique(bindings.begin(), bindings.end()) - bindings.begin();
        bindings.shrink_to(new_size);
    }

    const u32 max_set = std::accumulate(_bindings.begin(), _bindings.end(), 0, [](u32 max, const auto& p) { return std::max(max, p.first); });

    const usize max_variable_binding = std::accumulate(shaders.begin(), shaders.end(), 0_uu, [](usize sum, const ShaderModuleBase* s) { return sum + s->variable_size_bindings().size(); });
    core::ScratchVector<u32> variable_bindings(max_variable_binding);
    for(const ShaderModuleBase* shader : shaders) {
        merge(variable_bindings, shader->variable_size_bindings());
    }

    std::sort(variable_bindings.begin(), variable_bindings.end());
    const auto variable_bindings_end = std::unique(variable_bindings.begin(), variable_bindings.end());


    if(!_bindings.is_empty()) {
        _layouts = core::Vector<VkDescriptorSetLayout>(max_set + 1, VkDescriptorSetLayout{});
        for(const auto& binding : _bindings) {
            validate_bindings(binding.second);
            if(std::find(variable_bindings.begin(), variable_bindings_end, binding.first) != variable_bindings_end) {
                y_always_assert(binding.second.size() == 1, "Variable size descriptor bindings must be alone in descriptor set");
                _layouts[binding.first] = texture_library().descriptor_set_layout();
            } else {
                _layouts[binding.first] = descriptor_set_allocator().descriptor_set_layout(binding.second).vk_descriptor_set_layout();
            }
        }
    }
}

ShaderProgramBase::ShaderProgramBase(ShaderProgramBase&& other) {
    swap(other);
}

ShaderProgramBase& ShaderProgramBase::operator=(ShaderProgramBase&& other) {
    swap(other);
    return *this;
}

void ShaderProgramBase::swap(ShaderProgramBase& other) {
    std::swap(_bindings, other._bindings);
    std::swap(_layouts, other._layouts);
    std::swap(_stages, other._stages);
}

core::Span<VkPipelineShaderStageCreateInfo> ShaderProgramBase::vk_pipeline_stage_info() const {
    return _stages;
}

core::Span<VkDescriptorSetLayout> ShaderProgramBase::vk_descriptor_layouts() const {
    return _layouts;
}






ShaderProgram::ShaderProgram(const FragmentShader& frag, const VertexShader& vert, const GeometryShader& geom) :
    ShaderProgramBase(make_shader_array(frag, vert, geom)) {

    {
        auto vertex_attribs = core::ScratchPad<ShaderModuleBase::Attribute>(vert.attributes());
        std::sort(vertex_attribs.begin(), vertex_attribs.end(), [](const auto& a, const auto& b) { return a.location < b.location; });
        create_vertex_attribs(vertex_attribs, _vertex.bindings, _vertex.attribs);
    }
    {
        _fragment_outputs = frag.stage_output();
        std::sort(_fragment_outputs.begin(), _fragment_outputs.end());
    }
}

core::Span<VkVertexInputBindingDescription> ShaderProgram::vk_attribute_bindings() const {
    return _vertex.bindings;
}

core::Span<VkVertexInputAttributeDescription> ShaderProgram::vk_attributes_descriptions() const {
    return _vertex.attribs;
}

core::Span<u32> ShaderProgram::fragment_outputs() const {
    return _fragment_outputs;
}









RaytracingProgram::RaytracingProgram(const RayGenShader& gen, const MissShader& miss, const ClosestHitShader& chit) :
    ShaderProgramBase(make_shader_array(gen, miss, chit)) {

    const auto shaders = make_shader_array(gen, miss, chit);


    VkPipelineLayoutCreateInfo layout_create_info = vk_struct();
    {
        layout_create_info.pSetLayouts = _layouts.data();
        layout_create_info.setLayoutCount = u32(_layouts.size());
    }

    vk_check(vkCreatePipelineLayout(vk_device(), &layout_create_info, vk_allocation_callbacks(), _layout.get_ptr_for_init()));


    std::array<VkRayTracingShaderGroupCreateInfoKHR, 3> groups;
    for(usize i = 0; i != shaders.size(); ++i) {
        groups[i] = vk_struct();
        groups[i].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
        groups[i].generalShader = VK_SHADER_UNUSED_KHR;
        groups[i].closestHitShader = VK_SHADER_UNUSED_KHR;
        groups[i].anyHitShader = VK_SHADER_UNUSED_KHR;
        groups[i].intersectionShader = VK_SHADER_UNUSED_KHR;
    }

    {
        groups[0].generalShader = 0;
        groups[1].generalShader = 1;

        groups[2].closestHitShader = 2;
        groups[2].type = VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
    }

    VkRayTracingPipelineCreateInfoKHR create_info = vk_struct();
    {
        create_info.pStages = _stages.data();
        create_info.stageCount = u32(_stages.size());
        create_info.pGroups = groups.data();
        create_info.groupCount = u32(groups.size());

        create_info.maxPipelineRayRecursionDepth = 1;
        create_info.layout = _layout;
    }

    vk_check(vkCreateRayTracingPipelinesKHR(vk_device(), nullptr, nullptr, 1, &create_info, nullptr, _pipeline.get_ptr_for_init()));

    const u32 group_size = u32(groups.size());
    const u32 table_size = device_properties().shader_group_handle_size_aligned * group_size;

    core::ScratchPad<u8> table_data(table_size);
    vk_check(vkGetRayTracingShaderGroupHandlesKHR(vk_device(), _pipeline, 0, group_size, table_size, table_data.data()));

    _binding_tables = Buffer<BufferUsage::BindingTableBit, MemoryType::CpuVisible>(device_properties().shader_group_handle_size_base_aligned * group_size);
    {
        auto mapping = _binding_tables.map_bytes(MappingAccess::WriteOnly);

        for(u32 i = 0; i != group_size; ++i) {
            std::copy_n(
                table_data.data() + i * device_properties().shader_group_handle_size_aligned,
                device_properties().shader_group_handle_size,
                mapping.data() + i * device_properties().shader_group_handle_size_base_aligned
                );
        }
    }
}

RaytracingProgram::~RaytracingProgram() {
    destroy_graphic_resource(std::move(_layout));
    destroy_graphic_resource(std::move(_pipeline));
}

VkPipeline RaytracingProgram::vk_pipeline() const {
    return _pipeline;
}

VkPipelineLayout RaytracingProgram::vk_pipeline_layout() const {
    return _layout;
}


std::array<VkStridedDeviceAddressRegionKHR, 4> RaytracingProgram::vk_binding_tables() const {
    const VkDeviceAddress base_addr = vk_buffer_device_address(SubBuffer(_binding_tables));
    const u32 table_aligned_size = device_properties().shader_group_handle_size_base_aligned;

    std::array<VkStridedDeviceAddressRegionKHR, 4> tables = {};
    for(usize i = 0; i != 3; ++i) {
        tables[i].deviceAddress = base_addr + (i * table_aligned_size);
        tables[i].size = table_aligned_size;
        tables[i].stride = table_aligned_size;
    }

    return tables;
}



}

