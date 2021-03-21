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

#include "ShaderModuleBase.h"

#include <yave/graphics/graphics.h>

#include <external/spirv_cross/spirv.hpp>
#include <external/spirv_cross/spirv_cross.hpp>

namespace yave {

template<typename M>
static void merge(M& into, const M& o) {
    for(const auto& p : o) {
        into[p.first].push_back(p.second.begin(), p.second.end());
    }
}

static VkShaderModule create_shader_module(const SpirVData& data) {
    VkShaderModule shader = vk_null();
    if(data.is_empty()) {
        return shader;
    }

    VkShaderModuleCreateInfo create_info = vk_struct();
    {
        create_info.codeSize = data.size();
        create_info.pCode = data.data();
    }

    vk_check(vkCreateShaderModule(vk_device(), &create_info, vk_allocation_callbacks(), &shader));
    return shader;
}

static ShaderType module_type(const spirv_cross::Compiler& compiler) {
    switch(compiler.get_execution_model()) {
        case spv::ExecutionModelVertex:
            return ShaderType::Vertex;
        case spv::ExecutionModelFragment:
            return ShaderType::Fragment;
        case spv::ExecutionModelGeometry:
            return ShaderType::Geomery;
        case spv::ExecutionModelGLCompute:
            return ShaderType::Compute;

        default:
            break;
    }
    y_fatal("Unknown shader execution model.");
}


static bool is_inline(const spirv_cross::Compiler& compiler, const spirv_cross::Resource& res) {
    if(compiler.get_type(res.type_id).storage != spv::StorageClass::StorageClassUniform) {
        return false;
    }
    const std::string_view name = std::string_view(res.name);
    if(name.size() > 7 && name.substr(name.size() - 7) == "_Inline") {
        return true;
    }
    return false;
}

static VkDescriptorSetLayoutBinding create_binding(const spirv_cross::Compiler& compiler, const spirv_cross::Resource& res, VkDescriptorType type) {
    usize size = 1;
    if(is_inline(compiler, res)) {
        type = VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT;
        size = compiler.get_declared_struct_size(compiler.get_type(res.type_id));
    }
    VkDescriptorSetLayoutBinding binding = {};
    {
        binding.binding = compiler.get_decoration(res.id, spv::DecorationBinding);
        binding.descriptorCount = u32(size);
        binding.descriptorType = type;
        binding.stageFlags = VK_SHADER_STAGE_ALL;
    }
    return binding;
}



template<typename R>
static auto create_bindings(const spirv_cross::Compiler& compiler, const R& resources, ShaderType, VkDescriptorType type) {
    auto bindings = core::ExternalHashMap<u32, core::Vector<VkDescriptorSetLayoutBinding>>();
    for(const auto& res : resources) {
        const u32 set_index = compiler.get_decoration(res.id, spv::DecorationDescriptorSet);
        bindings[set_index] << create_binding(compiler, res, type);
    }
    return bindings;
}

template<typename R>
static void fail_not_empty(const R& res) {
    if(!res.empty()) {
        y_fatal("Unsupported resource type.");
    }
}

static u32 component_size(spirv_cross::SPIRType::BaseType type) {
    switch(type) {
        case spirv_cross::SPIRType::Float:
        case spirv_cross::SPIRType::Int:
        case spirv_cross::SPIRType::UInt:
            return 4;

        case spirv_cross::SPIRType::Char:
            return 1;

        default:
            break;
    }
    y_fatal("Unsupported attribute type.");
}

static ShaderModuleBase::AttribType component_type(spirv_cross::SPIRType::BaseType type) {
    switch(type) {
        case spirv_cross::SPIRType::Float:
            return ShaderModuleBase::AttribType::Float;

        case spirv_cross::SPIRType::Int:
            return ShaderModuleBase::AttribType::Int;

        case spirv_cross::SPIRType::UInt:
            return ShaderModuleBase::AttribType::Uint;

        case spirv_cross::SPIRType::Char:
            return ShaderModuleBase::AttribType::Char;

        default:
            break;
    }
    y_fatal("Unsupported attribute type.");
}

template<typename R>
static auto create_push_constants(const spirv_cross::Compiler& compiler, const R& resources, ShaderType shader_type) {
    if(resources.size() > 1) {
        y_fatal("Too many push constants.");
    }

    core::Vector<VkPushConstantRange> push_constants;
    for(const auto& r : resources) {
        const auto& type = compiler.get_type(r.type_id);
        push_constants << VkPushConstantRange{VkShaderStageFlags(shader_type), 0, u32(compiler.get_declared_struct_size(type))};
    }
    return push_constants;
}


template<typename R>
static auto create_attribs(const spirv_cross::Compiler& compiler, const R& resources) {
    core::Vector<ShaderModuleBase::Attribute> attribs;
    std::unordered_set<u32> locations;
    for(const auto& r : resources) {
        const auto location = compiler.get_decoration(r.id, spv::DecorationLocation);
        const auto& type = compiler.get_type(r.type_id);

        attribs << ShaderModuleBase::Attribute{location, type.columns, type.vecsize, component_size(type.basetype), component_type(type.basetype)};

        for(usize i = location; i != location + type.columns; ++i) {
            if(!locations.insert(u32(i)).second) {
                y_fatal("Duplicate or overlapping attribute locations.");
            }
        }
    }
    return attribs;
}


ShaderType ShaderModuleBase::shader_type(const SpirVData& data) {
    const spirv_cross::Compiler compiler(std::vector<u32>(data.data(), data.data() + data.size() / 4));
    return module_type(compiler);
}

ShaderModuleBase::ShaderModuleBase(const SpirVData& data) : _module(create_shader_module(data)) {
    const spirv_cross::Compiler compiler(std::vector<u32>(data.data(), data.data() + data.size() / 4));

    _type = module_type(compiler);

    auto resources = compiler.get_shader_resources();
    merge(_bindings, create_bindings(compiler, resources.uniform_buffers, _type, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER));
    merge(_bindings, create_bindings(compiler, resources.storage_buffers, _type, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER));
    merge(_bindings, create_bindings(compiler, resources.sampled_images, _type, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER));
    merge(_bindings, create_bindings(compiler, resources.storage_images, _type, VK_DESCRIPTOR_TYPE_STORAGE_IMAGE));

    /*auto print_resources = [&](auto resources) {
        for(const auto& buffer : resources) {
            const auto& type = compiler.get_type(buffer.base_type_id);
            log_msg(fmt("%:", buffer.name.data()), Log::Warning);
            log_msg(fmt("   storage: %", compiler.get_storage_class(buffer.id)), Log::Warning);
            log_msg(fmt("   type base: %", type.basetype), Log::Warning);
            log_msg(fmt("   type storage: %", type.storage), Log::Warning);
            const auto& bitset = compiler.get_decoration_bitset(buffer.id);
            bitset.for_each_bit([](u32 bit) {
                log_msg(fmt("   decoration: %", bit), Log::Warning);
            });
        }
    };

    print_resources(resources.uniform_buffers);
    print_resources(resources.storage_buffers);*/

    _push_constants = create_push_constants(compiler, resources.push_constant_buffers, _type);

    _attribs = create_attribs(compiler, resources.stage_inputs);

    // these are attribs & other stages stuff
    fail_not_empty(resources.atomic_counters);
    fail_not_empty(resources.separate_images);
    fail_not_empty(resources.separate_samplers);

    for(const auto& res : resources.stage_outputs) {
        _stage_output << compiler.get_decoration(res.id, spv::DecorationLocation);
    }

    u32 spec_offset = 0;
    for(const auto& cst : compiler.get_specialization_constants()) {
        const auto& type = compiler.get_type(compiler.get_constant(cst.id).constant_type);
        const u32 size = type.width / 8;
        _spec_constants << VkSpecializationMapEntry{cst.constant_id, spec_offset, size};
        spec_offset += size;
    }

    for(u32 i = 0; i != 3; ++i) {
        _local_size[i] = compiler.get_execution_mode_argument(spv::ExecutionMode::ExecutionModeLocalSize, i);
    }
}

ShaderModuleBase::~ShaderModuleBase() {
    device_destroy(_module);
}

}

