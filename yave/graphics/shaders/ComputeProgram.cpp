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

#include "ComputeProgram.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/descriptors/DescriptorSetAllocator.h>

#include <y/core/ScratchPad.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

#include <numeric>

namespace yave {

ComputeProgram::ComputeProgram(const ComputeShader& comp) : _local_size(comp.local_size()) {
    y_profile();

    const auto& bindings = comp.bindings();
    core::ScratchPad<VkDescriptorSetLayout> layouts(bindings.size());

    y_always_assert(bindings.size() <= 1, "Multiple descriptor sets are not supported");

    for(usize i = 0; i != bindings.size(); ++i) {
        const auto& layout = descriptor_set_allocator().descriptor_set_layout(bindings[i]);
        layouts[i] = layout.vk_push_descriptor_set_layout();
    }

    VkPipelineLayoutCreateInfo layout_create_info = vk_struct();
    {
        layout_create_info.pSetLayouts = layouts.data();
        layout_create_info.setLayoutCount = u32(layouts.size());
    }

    vk_check(vkCreatePipelineLayout(vk_device(), &layout_create_info, vk_allocation_callbacks(), _layout.get_ptr_for_init()));

    VkPipelineShaderStageCreateInfo stage = vk_struct();
    {
        stage.module = comp.vk_shader_module();
        stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        stage.pName = comp.entry_point().data();
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

