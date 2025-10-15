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

#include "DescriptorArray.h"

#include <yave/graphics/device/DebugUtils.h>
#include <yave/graphics/device/DeviceProperties.h>

#include <y/core/ScratchPad.h>
#include <y/utils/format.h>

#include <vulkan/vk_enum_string_helper.h>

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
        const u32 max_descs = max_descriptor_of_type(type);
        y_always_assert(max_descs > 2 * ManagedDescriptorArray::reserved_descriptor_count, "not enough descriptors of type {}", string_VkDescriptorType(type));

        binding.descriptorType = type;
        binding.descriptorCount = std::min(max_descs - ManagedDescriptorArray::reserved_descriptor_count, ManagedDescriptorArray::upper_descriptor_count_limit);
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






DescriptorArray::DescriptorArray(VkDescriptorType type, u32 starting_capacity) : _layout(create_libray_layout(type)), _type(type) {

    alloc_set(starting_capacity);

#ifdef Y_DEBUG
    if(const auto* debug = debug_utils()) {
        debug->set_resource_name(_set, fmt_c_str("Descriptor Array [{}]", _type));
    }
#endif
}

DescriptorArray::~DescriptorArray() {
    destroy_graphic_resource(std::move(_pool));
    destroy_graphic_resource(std::move(_layout));
}

VkDescriptorType DescriptorArray::descriptor_type() const {
    return _type;
}

DescriptorSetProxy DescriptorArray::descriptor_set() const {
    return DescriptorSetProxy(_set);
}

VkDescriptorSetLayout DescriptorArray::descriptor_set_layout() const {
    return _layout;
}

void DescriptorArray::alloc_set(u32 size) {
    y_profile();

    y_debug_assert(size > _capacity);

    VkHandle<VkDescriptorPool> new_pool = create_libray_pool(size, _type);
    const VkDescriptorSet new_set = create_array_set(new_pool, _layout, size);

    if(_capacity) {
        VkCopyDescriptorSet copy = vk_struct();
        {
            copy.srcSet = _set;
            copy.dstSet = new_set;
            copy.descriptorCount = _capacity;
        }

        vkUpdateDescriptorSets(vk_device(), 0, nullptr, 1, &copy);
    }

    for(u32 i = _capacity; i < size; ++i) {
        _free << size - i - 1;
    }

    _capacity = size;
    _set = new_set;
    destroy_graphic_resource(std::exchange(_pool, std::move(new_pool)));
}

u32 DescriptorArray::add_descriptor(const Descriptor& desc) {
    y_debug_assert(desc.vk_descriptor_type() == _type);

    const auto lock = std::unique_lock(_set_lock);

    if(_free.is_empty()) {
        alloc_set(2 << log2ui(_capacity + 1));
    }

    const u32 index = _free.pop();
    add_descriptor_to_set(desc, index);
    return index;
}

void DescriptorArray::remove_descriptor(u32 index) {
    const auto lock = std::unique_lock(_set_lock);

    y_debug_assert(std::find(_free.begin(), _free.end(), index) == _free.end());
    _free << index;
}

void DescriptorArray::add_descriptor_to_set(const Descriptor& desc, u32 index) {
    const VkWriteDescriptorSet write = descriptor_write(_set, desc, index);
    vkUpdateDescriptorSets(vk_device(), 1, &write, 0, nullptr);
}

VkWriteDescriptorSet DescriptorArray::descriptor_write(VkDescriptorSet set, const Descriptor& desc, u32 index) const {
    VkWriteDescriptorSet write = vk_struct();
    {
        write.descriptorType = _type;
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.dstSet = set;
    }

    if(Descriptor::is_buffer(_type)) {
        write.pBufferInfo = &desc.descriptor_info().buffer;
    } else if(Descriptor::is_image(_type)) {
        write.pImageInfo = &desc.descriptor_info().image;
    } else {
        y_fatal("Incompatible descriptor type");
    }

    return write;
}






ManagedDescriptorArray::ManagedDescriptorArray(VkDescriptorType type, u32 starting_capacity) : DescriptorArray(type, starting_capacity) {
}

ManagedDescriptorArray::~ManagedDescriptorArray() {
    y_always_assert(_descriptors.is_empty(), "Some bindless descriptors have not been released"); // Do we care?
}

ManagedDescriptorArray::DescriptorKey ManagedDescriptorArray::descriptor_key(const Descriptor& desc) {
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

u32 ManagedDescriptorArray::add_descriptor_managed(const Descriptor& desc) {
    y_debug_assert(desc.vk_descriptor_type() == descriptor_type());

    const DescriptorKey key = descriptor_key(desc);

    const auto lock = std::unique_lock(_set_lock);

    Entry& entry = _descriptors[key];
    if(entry.ref_count) {
        ++entry.ref_count;
        return entry.index;
    }

    entry.ref_count = 1;
    entry.index = add_descriptor(desc);

    return entry.index;
}

void ManagedDescriptorArray::remove_descriptor_managed(const Descriptor& desc) {
    y_debug_assert(desc.vk_descriptor_type() == descriptor_type());

    const DescriptorKey key = descriptor_key(desc);

    const auto lock = std::unique_lock(_set_lock);

    const auto it = _descriptors.find(key);
    y_debug_assert(it != _descriptors.end());

    if(--(it->second.ref_count) == 0) {
        remove_descriptor(it->second.index);
        _descriptors.erase(it);
    }
}


}


