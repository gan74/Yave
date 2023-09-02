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

#include "DescriptorArray.h"

#include <yave/graphics/device/extensions/DebugUtils.h>

#include <y/utils/format.h>

namespace yave {

static VkHandle<VkDescriptorPool> create_libray_pool(u32 desc_count, VkDescriptorType type) {
    VkDescriptorPoolSize pool_size;
    {
        pool_size.type = type;
        pool_size.descriptorCount = desc_count;
    }

    VkDescriptorPoolCreateInfo create_info = vk_struct();
    {
        create_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
        create_info.poolSizeCount = 1;
        create_info.pPoolSizes = &pool_size;
        create_info.maxSets = 1;
    }

    VkHandle<VkDescriptorPool> pool;
    vk_check(vkCreateDescriptorPool(vk_device(), &create_info, vk_allocation_callbacks(), pool.get_ptr_for_init()));
    return pool;
}

static VkHandle<VkDescriptorSetLayout> create_libray_layout(u32 desc_count, VkDescriptorType type) {
    const VkDescriptorBindingFlags binding_flags =
            VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
            VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
            VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT |
            VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT;

    VkDescriptorSetLayoutBindingFlagsCreateInfo flags = vk_struct();
    {
        flags.bindingCount = 1;
        flags.pBindingFlags = &binding_flags;
    }

    VkDescriptorSetLayoutBinding binding = {};
    {
        binding.descriptorType = type;
        binding.descriptorCount = desc_count;
        binding.stageFlags = VK_SHADER_STAGE_ALL;
    }

    VkDescriptorSetLayoutCreateInfo create_info = vk_struct();
    {
        create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        create_info.pBindings = &binding;
        create_info.bindingCount = 1;
        create_info.pNext = &flags;
    }

    VkHandle<VkDescriptorSetLayout> layout;
    vk_check(vkCreateDescriptorSetLayout(vk_device(), &create_info, vk_allocation_callbacks(), layout.get_ptr_for_init()));
    return layout;
}



DescriptorArray::ArraySet::ArraySet(VkDescriptorPool pool, VkDescriptorSetLayout layout, u32 desc_count) {
    VkDescriptorSetVariableDescriptorCountAllocateInfo variable = vk_struct();
    {
        variable.descriptorSetCount = 1;
        variable.pDescriptorCounts = &desc_count;
    }

    VkDescriptorSetAllocateInfo allocate_info = vk_struct();
    {
        allocate_info.pNext = &variable;
        allocate_info.descriptorPool = pool;
        allocate_info.descriptorSetCount = 1;
        allocate_info.pSetLayouts = &layout;
    }

    vk_check(vkAllocateDescriptorSets(vk_device(), &allocate_info, &_set));
}


DescriptorArray::DescriptorArray(VkDescriptorType type, u32 max_desc) :
        _pool(create_libray_pool(max_desc, type)),
        _layout(create_libray_layout(max_desc, type)),
        _set(_pool, _layout, max_desc),
        _max_descriptors(max_desc),
        _type(type) {

#ifdef Y_DEBUG
    if(const auto* debug = debug_utils()) {
        debug->set_resource_name(_set.vk_descriptor_set(), fmt_c_str("Descriptor Array [{}]", _type));
    }
#endif
}

DescriptorArray::~DescriptorArray() {
    {
        const auto lock = y_profile_unique_lock(_map_lock);
        y_always_assert(_descriptors.is_empty(), "Some bindless descriptors have not been released"); // Do we care?
    }
    destroy_graphic_resource(std::move(_pool));
    destroy_graphic_resource(std::move(_layout));
}

DescriptorArray::DescriptorKey DescriptorArray::descriptor_key(const Descriptor& desc) {
    DescriptorKey key = {};

    const auto& info = desc.descriptor_info();
    if(desc.is_buffer()) {
        key.buffer.buffer = info.buffer.buffer;
        key.buffer.offset = u64(info.buffer.offset);
    } else if(desc.is_image()) {
        key.image.view = info.image.imageView;
        key.image.sampler = info.image.sampler;
    } else {
        y_fatal("Incompatible descriptor type");
    }

    return key;
}

u32 DescriptorArray::add_descriptor(const Descriptor& desc) {
    y_debug_assert(desc.vk_descriptor_type() == _type);

    u32 entry_index = 0;
    {
        const auto lock = y_profile_unique_lock(_map_lock);
        Entry& entry = _descriptors[descriptor_key(desc)];
        if(entry.ref_count) {
            ++entry.ref_count;
            return entry.index;
        }

        entry.ref_count = 1;
        entry_index = entry.index = _free.is_empty() ?  u32(_descriptors.size() - 1) : _free.pop();
    }

    add_descriptor_to_set(desc, entry_index);

    return entry_index;
}

void DescriptorArray::remove_descriptor(const Descriptor& desc) {
    y_debug_assert(desc.vk_descriptor_type() == _type);

    const auto lock = y_profile_unique_lock(_map_lock);

    const auto it = _descriptors.find(descriptor_key(desc));
    y_debug_assert(it != _descriptors.end());

    if(--(it->second.ref_count) == 0) {
        _free.push_back(it->second.index);
        _descriptors.erase(it);
    }
}

void DescriptorArray::add_descriptor_to_set(const Descriptor& desc, u32 index) {
    y_debug_assert(desc.vk_descriptor_type() == _type);

    const auto lock = y_profile_unique_lock(_set_lock);

    VkWriteDescriptorSet write = vk_struct();
    {
        write.descriptorType = _type;
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.dstSet = _set.vk_descriptor_set();
    }

    if(desc.is_buffer()) {
        write.pBufferInfo = &desc.descriptor_info().buffer;
    } else if(desc.is_image()) {
        write.pImageInfo = &desc.descriptor_info().image;
    } else {
        y_fatal("Incompatible descriptor type");
    }

    vkUpdateDescriptorSets(vk_device(), 1, &write, 0, nullptr);
}

usize DescriptorArray::descriptor_count() const {
    const auto lock = y_profile_unique_lock(_map_lock);
    return _descriptors.size();
}

DescriptorSetBase DescriptorArray::descriptor_set() const {
    return _set;
}

VkDescriptorSetLayout DescriptorArray::descriptor_set_layout() const {
    return _layout;
}


}


