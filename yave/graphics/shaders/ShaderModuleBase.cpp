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

#include "ShaderModuleBase.h"

#include <yave/graphics/graphics.h>

#include <y/core/ScratchPad.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <external/spirv_reflect/spirv_reflect.h>

namespace yave {

static VkHandle<VkShaderModule> create_shader_module(const SpirVData& spirv) {
    const core::Span<u32> data = spirv.data();

    VkHandle<VkShaderModule> shader;
    if(data.is_empty()) {
        return shader;
    }

    VkShaderModuleCreateInfo create_info = vk_struct();
    {
        create_info.codeSize = data.size() * sizeof(u32);
        create_info.pCode = data.data();
    }

    vk_check(vkCreateShaderModule(vk_device(), &create_info, vk_allocation_callbacks(), shader.get_ptr_for_init()));
    return shader;
}


static bool is_inline(const SpvReflectDescriptorBinding& binding) {
    Y_TODO(check if inline descriptors are always at the end)
    return
        binding.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER &&
        binding.type_description->type_name &&
        (std::string_view(binding.type_description->type_name).ends_with("_Inline") || std::string_view(binding.name).ends_with("_Inline"));
}

static ShaderType shader_exec_model(SpvExecutionModel exec_model) {
    switch(exec_model) {
        case SpvExecutionModelVertex:               return ShaderType::Vertex;
        case SpvExecutionModelGeometry:             return ShaderType::Geomery;
        case SpvExecutionModelFragment:             return ShaderType::Fragment;
        case SpvExecutionModelGLCompute:            return ShaderType::Compute;
        case SpvExecutionModelRayGenerationKHR:     return ShaderType::RayGen;
        case SpvExecutionModelMissKHR:              return ShaderType::Miss;
        case SpvExecutionModelClosestHitKHR:        return ShaderType::ClosestHit;

        default:
        break;
    }
    y_fatal("Unknown shader execution model");
}

static void spv_check(SpvReflectResult result) {
    y_always_assert(result == SPV_REFLECT_RESULT_SUCCESS, "SpirV-Reflect error");
}

[[maybe_unused]]
static const SpvReflectEntryPoint& find_entry_point(const SpvReflectShaderModule& module, const core::String& name) {
    for(u32 i = 0; i != module.entry_point_count; ++i) {
        if(name == module.entry_points[i].name) {
            return module.entry_points[i];
        }
    }

    y_fatal("SpirV entry point not found");
}

[[maybe_unused]]
static const SpvReflectEntryPoint& find_entry_point(const SpvReflectShaderModule& module, ShaderType type) {
    y_debug_assert(type != ShaderType::None);
    for(u32 i = 0; i != module.entry_point_count; ++i) {
        if(shader_exec_model(module.entry_points[i].spirv_execution_model) == type) {
            return module.entry_points[i];
        }
    }

    y_fatal("SpirV entry point not found for shader type");
}




ShaderModuleBase::ShaderModuleBase(const SpirVData& spirv, ShaderType type) : _module(create_shader_module(spirv)), _type(type) {
    y_profile();

    y_debug_assert(_type != ShaderType::None);

    SpvReflectShaderModule module = {};

    const core::Span<u32> data = spirv.data();
    spv_check(spvReflectCreateShaderModule(data.size() * sizeof(u32), data.data(), &module));
    y_defer(spvReflectDestroyShaderModule(&module));

    const SpvReflectEntryPoint& entry_point = find_entry_point(module, type);

    _entry_point = entry_point.name;

    _local_size = {
        entry_point.local_size.x,
        entry_point.local_size.y,
        entry_point.local_size.z,
    };

    {
        u32 ds_count = 0;
        spv_check(spvReflectEnumerateDescriptorSets(&module, &ds_count, nullptr));

        core::ScratchPad<SpvReflectDescriptorSet*> sets(ds_count);
        spv_check(spvReflectEnumerateDescriptorSets(&module, &ds_count, sets.data()));

        for(const SpvReflectDescriptorSet* set : sets) {
            auto& set_bindings = _bindings[set->set];
            for(u32 i = 0; i != set->binding_count; ++i) {
                const SpvReflectDescriptorBinding& refl_binding = *set->bindings[i];

                y_always_assert(refl_binding.block.size == refl_binding.block.padded_size, "Invalid block size");
                y_debug_assert(refl_binding.count <= 1);

                if(!refl_binding.count) {
                    _variable_size_bindings << set->set;
                }

                VkDescriptorSetLayoutBinding& binding = set_bindings.emplace_back();
                {
                    binding.stageFlags = VK_SHADER_STAGE_ALL;
                    binding.binding = refl_binding.binding;
                    binding.descriptorCount = 1;
                    binding.descriptorType = VkDescriptorType(refl_binding.descriptor_type);
                }

                if(is_inline(refl_binding)) {
                    // For some reason refl_binding.block.padded_size gives 16 bytes blocks when the shader could do with 4 or 8
                    // We recompute size for blocks with members
                    if(refl_binding.block.members) {
                        u32 desc_size = 0;
                        for(u32 k = 0; k != refl_binding.block.member_count; ++k) {
                            const SpvReflectBlockVariable& member = refl_binding.block.members[k];
                            desc_size = std::max(desc_size, member.offset + member.size);
                        }
                        binding.descriptorCount *= desc_size;
                    } else {
                        binding.descriptorCount *= refl_binding.block.padded_size;
                    }

                    binding.descriptorType = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
                }
            }

            std::sort(set_bindings.begin(), set_bindings.end(), [](const auto& a, const auto& b) { return a.binding < b.binding; });
        }
    }


    {
        u32 attrib_count = 0;
        spv_check(spvReflectEnumerateEntryPointInputVariables(&module, entry_point.name, &attrib_count, nullptr));

        core::ScratchPad<SpvReflectInterfaceVariable*> attribs(attrib_count);
        spv_check(spvReflectEnumerateEntryPointInputVariables(&module, entry_point.name, &attrib_count, attribs.data()));

        for(const SpvReflectInterfaceVariable* variable : attribs) {
            if(variable->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
                continue;
            }

            Attribute& attrib = _attribs.emplace_back();
            {
                attrib.component_count = std::max(1u, variable->numeric.matrix.column_count);
                attrib.location = variable->location;
                attrib.format = VkFormat(variable->format);
                attrib.is_packed = std::string_view(variable->name).ends_with("_Packed");
            }
        }
    }

    {
        u32 out_count = 0;
        spv_check(spvReflectEnumerateEntryPointOutputVariables(&module, entry_point.name, &out_count, nullptr));

        core::ScratchPad<SpvReflectInterfaceVariable*> outputs(out_count);
        spv_check(spvReflectEnumerateEntryPointOutputVariables(&module, entry_point.name, &out_count, outputs.data()));

        for(const SpvReflectInterfaceVariable* variable : outputs) {
            if(variable->decoration_flags & SPV_REFLECT_DECORATION_BUILT_IN) {
                continue;
            }

            _stage_output.emplace_back(variable->location);
        }
    }

}

ShaderModuleBase::~ShaderModuleBase() {
    destroy_graphic_resource(std::move(_module));
}

}

