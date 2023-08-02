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

#include "TextureLibrary.h"

namespace yave {


static VkHandle<VkDescriptorPool> create_libray_pool(u32 desc_count) {
    VkDescriptorPoolSize pool_size;
    {
        pool_size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
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

static VkHandle<VkDescriptorSetLayout> create_libray_layout(u32 desc_count) {
    const VkDescriptorBindingFlags binding_flags = VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT | VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT;

    VkDescriptorSetLayoutBindingFlagsCreateInfo flags = vk_struct();
    {
        flags.bindingCount = 1;
        flags.pBindingFlags = &binding_flags;
    }

    VkDescriptorSetLayoutBinding binding = {};
    {
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = desc_count;
        binding.stageFlags = VK_SHADER_STAGE_ALL;
    }

    VkDescriptorSetLayoutCreateInfo create_info = vk_struct();
    {
        create_info.flags = VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
        create_info.pBindings = &binding;
        create_info.bindingCount = 1;
    }

    VkHandle<VkDescriptorSetLayout> layout;
    vk_check(vkCreateDescriptorSetLayout(vk_device(), &create_info, vk_allocation_callbacks(), layout.get_ptr_for_init()));
    return layout;
}

TextureLibrary::LibrarySet::LibrarySet(VkDescriptorPool pool, VkDescriptorSetLayout layout, u32 desc_count) {
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

TextureLibrary::TextureLibrary() : _pool(create_libray_pool(_max_desc_count)), _layout(create_libray_layout(_max_desc_count)), _set(_pool, _layout, _max_desc_count) {
}

TextureLibrary::~TextureLibrary() {
    {
        const auto lock = y_profile_unique_lock(_map_lock);
        y_always_assert(_textures.is_empty(), "Some textures have not been released"); // Do we care?
    }
    destroy_graphic_resource(std::move(_pool));
    destroy_graphic_resource(std::move(_layout));
}

u32 TextureLibrary::add_texture(const TextureView& tex) {
    y_debug_assert(!tex.is_null());

    u32 entry_index = 0;
    {
        const auto lock = y_profile_unique_lock(_map_lock);
        Entry& entry = _textures[tex.vk_view()];
        if(entry.ref_count) {
            ++entry.ref_count;
            return entry.index;
        }

        entry.ref_count = 1;
        entry_index = entry.index =_free.is_empty() ?  u32(_textures.size() - 1) : _free.pop();
    }

    add_texture_to_set(tex, entry_index);

    return entry_index;
}

void TextureLibrary::remove_texture(const TextureView& tex) {
    y_debug_assert(!tex.is_null());

    const auto lock = y_profile_unique_lock(_map_lock);

    const auto it = _textures.find(tex.vk_view());
    y_debug_assert(it != _textures.end());

    if(--(it->second.ref_count) == 0) {
        _free.push_back(it->second.index);
        _textures.erase(it);
    }
}


void TextureLibrary::add_texture_to_set(const TextureView& tex, u32 index) {
    const auto lock = y_profile_unique_lock(_set_lock);

    y_debug_assert(Descriptor(tex).vk_descriptor_type() == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    const VkDescriptorImageInfo image_info = Descriptor(tex).descriptor_info().image;

    VkWriteDescriptorSet write = vk_struct();
    {
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &image_info;
        write.dstArrayElement = index;
        write.descriptorCount = 1;
        write.dstSet = _set.vk_descriptor_set();
    }

    vkUpdateDescriptorSets(vk_device(), 1, &write, 0, nullptr);
}

usize TextureLibrary::texture_count() const {
    const auto lock = y_profile_unique_lock(_map_lock);
    return _textures.size();
}

DescriptorSetBase TextureLibrary::descriptor_set() const {
    return _set;
}


}


