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

#include "ComputeProgram.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/descriptors/DescriptorSetAllocator.h>

#include <y/core/ScratchPad.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

#include <numeric>

namespace yave {

ComputeProgram::ComputeProgram(const ComputeShader& comp, const SpecializationData& data) : _local_size(comp.local_size()) {
    const auto& bindings = comp.bindings();

    const u32 max_set = std::accumulate(bindings.begin(), bindings.end(), 0, [](u32 max, const auto& p) { return std::max(max, p.first); });

    core::ScratchPad<VkDescriptorSetLayout> layouts(max_set + 1);
    for(const auto& binding : bindings) {
        layouts[binding.first] = descriptor_set_allocator().descriptor_set_layout(binding.second).vk_descriptor_set_layout();
    }

    VkPipelineLayoutCreateInfo layout_create_info = vk_struct();
    {
        layout_create_info.pSetLayouts = layouts.data();
        layout_create_info.setLayoutCount = u32(layouts.size());
    }

    vk_check(vkCreatePipelineLayout(vk_device(), &layout_create_info, vk_allocation_callbacks(), _layout.get_ptr_for_init()));

    if(data.size() && data.size() != comp.specialization_data_size()) {
        y_fatal("Incompatible specialization data.");
    }

    const auto entries = data.size() ? comp.specialization_entries() : core::Span<VkSpecializationMapEntry>();

    VkSpecializationInfo spec_info = {};
    {
        spec_info.mapEntryCount = u32(entries.size());
        spec_info.pMapEntries = entries.data();
        spec_info.dataSize = u32(data.size());
        spec_info.pData = data.data();
    }

    VkPipelineShaderStageCreateInfo stage = vk_struct();
    {
        stage.module = comp.vk_shader_module();
        stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stage.pName = "main";
        stage.pSpecializationInfo = &spec_info;
    }

    VkComputePipelineCreateInfo create_info = vk_struct();
    {
        create_info.layout = _layout;
        create_info.stage = stage;
    }

    vk_check(vkCreateComputePipelines(vk_device(), {}, 1, &create_info, vk_allocation_callbacks(), _pipeline.get_ptr_for_init()));
}

ComputeProgram::~ComputeProgram() {
    destroy_graphic_resource(std::move(_layout));
    destroy_graphic_resource(std::move(_pipeline));
}

bool ComputeProgram::is_null() const {
    return !_pipeline;
}

const math::Vec3ui& ComputeProgram::local_size() const {
    return _local_size;
}

usize ComputeProgram::thread_count() const {
    return _local_size.x() * _local_size.y() * _local_size.z();
}

VkPipeline ComputeProgram::vk_pipeline() const {
    y_debug_assert(!is_null());
    return _pipeline;
}

VkPipelineLayout ComputeProgram::vk_pipeline_layout() const {
    y_debug_assert(!is_null());
    return _layout;
}

}

