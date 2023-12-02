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
#include <yave/graphics/device/DeviceProperties.h>

#include <y/core/ScratchPad.h>
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

static u32 max_descriptor_of_type(VkDescriptorType type) {
    const DeviceProperties& props = device_properties();
    switch(type) {
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            return props.max_sampled_image_desc_array_size;

        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return props.max_storage_image_desc_array_size;

        case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            return props.max_uniform_buffer_desc_array_size;

        case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            return props.max_storage_buffer_desc_array_size;

        default:
        break;
    }
    y_fatal("Unsupported descriptor type");
}

static VkHandle<VkDescriptorSetLayout> create_libray_layout(VkDescriptorType type) {
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
        binding.descriptorCount = std::min(max_descriptor_of_type(type), DescriptorArray::upper_descriptor_count_limit);
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

static VkDescriptorSet create_array_set(VkDescriptorPool pool, VkDescriptorSetLayout layout, u32 desc_count) {
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

    VkDescriptorSet set = {};
    vk_check(vkAllocateDescriptorSets(vk_device(), &allocate_info, &set));
    return set;
}

DescriptorArray::DescriptorArray(VkDescriptorType type, u32 starting_capacity) :
        _layout(create_libray_layout(type)),
        _type(type) {

    allocator.locked([&](auto&& allocator) {
        _set = allocator.alloc_set(starting_capacity, this);
    });

#ifdef Y_DEBUG
    if(const auto* debug = debug_utils()) {
        debug->set_resource_name(_set, fmt_c_str("Descriptor Array [{}]", _type));
    }
#endif
}

DescriptorArray::~DescriptorArray() {
    allocator.locked([&](auto&& allocator) {
        y_always_assert(allocator.descriptors.is_empty(), "Some bindless descriptors have not been released"); // Do we care?
        destroy_graphic_resource(std::move(allocator.pool));
    });
    destroy_graphic_resource(std::move(_layout));
}

VkDescriptorSet DescriptorArray::Allocator::alloc_set(u32 size, const DescriptorArray* parent) {
    y_profile();
    y_debug_assert(size > capacity);

    destroy_graphic_resource(std::exchange(pool, create_libray_pool(size, parent->_type)));
    capacity = size;

    const VkDescriptorSet set = create_array_set(pool, parent->_layout, size);
    if(!descriptors.is_empty()) {
        core::ScratchVector<VkWriteDescriptorSet> writes(descriptors.size());
        for(const auto& [key, entry] : descriptors) {
            writes.emplace_back(parent->descriptor_write(set, key, entry.index));
        }

        vkUpdateDescriptorSets(vk_device(), u32(writes.size()), writes.data(), 0, nullptr);
    }
    return set;
}


DescriptorArray::DescriptorKey DescriptorArray::descriptor_key(const Descriptor& desc) {
    DescriptorKey key;

    const auto& info = desc.descriptor_info();
    if(desc.is_buffer()) {
        key.buffer = info.buffer;
    } else if(desc.is_image()) {
        key.image = info.image;
    } else {
        y_fatal("Incompatible descriptor type");
    }

    return key;
}

u32 DescriptorArray::add_descriptor(const Descriptor& desc) {
    y_debug_assert(desc.vk_descriptor_type() == _type);

    const DescriptorKey key = descriptor_key(desc);
    const u32 entry_index = allocator.locked([&](auto&& allocator) {
        if(allocator.free.is_empty() && allocator.descriptors.size() == allocator.capacity) {
            const VkDescriptorSet new_set = allocator.alloc_set(2 << log2ui(allocator.capacity + 1), this);
            const auto lock = std::unique_lock(_set_lock);
            _set = new_set;
        }


        Entry& entry = allocator.descriptors[key];
        if(entry.ref_count) {
            ++entry.ref_count;
            return entry.index;
        }

        entry.ref_count = 1;
        entry.index = allocator.free.is_empty() ?  u32(allocator.descriptors.size() - 1) : allocator.free.pop();
        return entry.index;
    });

    add_descriptor_to_set(key, entry_index);

    return entry_index;
}

void DescriptorArray::remove_descriptor(const Descriptor& desc) {
    y_debug_assert(desc.vk_descriptor_type() == _type);

    const DescriptorKey key = descriptor_key(desc);
    allocator.locked([&](auto&& allocator) {
        const auto it = allocator.descriptors.find(key);
        y_debug_assert(it != allocator.descriptors.end());

        if(--(it->second.ref_count) == 0) {
            allocator.free.push_back(it->second.index);
            allocator.descriptors.erase(it);
        }
    });
}

VkWriteDescriptorSet DescriptorArray::descriptor_write(VkDescriptorSet set, const DescriptorKey& key, u32 index) const {
    VkWriteDescriptorSet write = vk_struct();
    {
        write.descriptorType = _type;
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.dstSet = set;
    }

    if(Descriptor::is_buffer(_type)) {
        write.pBufferInfo = &key.buffer;
    } else if(Descriptor::is_image(_type)) {
        write.pImageInfo = &key.image;
    } else {
        y_fatal("Incompatible descriptor type");
    }

    return write;
}

void DescriptorArray::add_descriptor_to_set(const DescriptorKey& desc, u32 index) {
    const VkWriteDescriptorSet write = descriptor_write(_set, desc, index);

    const auto lock = std::unique_lock(_set_lock);
    vkUpdateDescriptorSets(vk_device(), 1, &write, 0, nullptr);
}

usize DescriptorArray::descriptor_count() const {
    return allocator.locked([&](auto&& allocator) { return allocator.descriptors.size(); });
}

DescriptorSetBase DescriptorArray::descriptor_set() const {
    return DescriptorArraySet(_set);
}

VkDescriptorSetLayout DescriptorArray::descriptor_set_layout() const {
    return _layout;
}


}


