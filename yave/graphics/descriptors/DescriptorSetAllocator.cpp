﻿/*******************************
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

#include "DescriptorSetAllocator.h"
#include "Descriptor.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/device/DeviceProperties.h>

#include <y/core/Range.h>
#include <y/core/ScratchPad.h>
#include <y/utils/memory.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

#include <bit>

namespace yave {

static constexpr usize inline_block_index = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1;
static constexpr usize accel_struct_index = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 2;

static usize descriptor_type_index(VkDescriptorType type) {
    if(Descriptor::is_inline_block(type)) {
        static_assert(inline_block_index < DescriptorSetLayout::descriptor_type_count);
        return inline_block_index;
    }

    if(Descriptor::is_acceleration_structure(type)) {
        static_assert(accel_struct_index < DescriptorSetLayout::descriptor_type_count);
        return accel_struct_index;
    }

    y_debug_assert(usize(type) < DescriptorSetLayout::descriptor_type_count);
    return usize(type);
}

static VkDescriptorType index_descriptor_type(usize index) {
    y_debug_assert(index <  DescriptorSetLayout::descriptor_type_count);

    if(index == inline_block_index) {
        return VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK;
    }

    if(index == accel_struct_index) {
        return VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    }

    return VkDescriptorType(index);
}

DescriptorSetLayout::DescriptorSetLayout(core::Span<VkDescriptorSetLayoutBinding> bindings) {
    for(const auto& b : bindings) {
        if(Descriptor::is_inline_block(b.descriptorType)) {
            ++_inline_blocks;
        }
        _sizes[descriptor_type_index(b.descriptorType)] += b.descriptorCount;
    }

    VkDescriptorSetLayoutCreateInfo create_info = vk_struct();
    {
        create_info.bindingCount = u32(bindings.size());
        create_info.pBindings = bindings.data();
    }
    vk_check(vkCreateDescriptorSetLayout(vk_device(), &create_info, vk_allocation_callbacks(), _layout.get_ptr_for_init()));
}

DescriptorSetLayout::~DescriptorSetLayout() {
    destroy_graphic_resource(std::move(_layout));
}

bool DescriptorSetLayout::is_null() const {
    return !_layout;
}

const std::array<u32, DescriptorSetLayout::descriptor_type_count>& DescriptorSetLayout::desciptors_count() const {
    return _sizes;
}

usize DescriptorSetLayout::inline_blocks() const {
    return _inline_blocks;
}

VkDescriptorSetLayout DescriptorSetLayout::vk_descriptor_set_layout() const {
    return _layout;
}



static VkHandle<VkDescriptorPool> create_descriptor_pool(const DescriptorSetLayout& layout, usize set_count) {
    y_profile();

    usize sizes_count = 0;
    std::array<VkDescriptorPoolSize, DescriptorSetLayout::descriptor_type_count> sizes;

    for(usize i = 0; i != DescriptorSetLayout::descriptor_type_count; ++i) {
        const usize type_count = layout.desciptors_count()[i];
        if(type_count) {
            VkDescriptorPoolSize& pool_size = sizes[sizes_count++];
            pool_size.type = index_descriptor_type(i);
            pool_size.descriptorCount = u32(type_count * set_count);
        }
    }

    if(!sizes_count) {
        return {};
    }

    VkDescriptorPoolCreateInfo create_info = vk_struct();
    {
        create_info.poolSizeCount = u32(sizes_count);
        create_info.pPoolSizes = sizes.data();
        create_info.maxSets = u32(set_count);
    }

    // This is apparently optional: see https://registry.khronos.org/vulkan/specs/1.3-extensions/man/html/VkDescriptorPoolInlineUniformBlockCreateInfo.html
    // Or not? Causes out of pool memory on Intel
    VkDescriptorPoolInlineUniformBlockCreateInfo inline_create_info = vk_struct();
    if(const u32 inline_blocks = u32(layout.inline_blocks())) {
        inline_create_info.maxInlineUniformBlockBindings = inline_blocks * u32(set_count);
        create_info.pNext = &inline_create_info;
    }

    VkHandle<VkDescriptorPool> pool;
    vk_check(vkCreateDescriptorPool(vk_device(), &create_info, vk_allocation_callbacks(), pool.get_ptr_for_init()));
    return pool;
}

DescriptorSetPool::DescriptorSetPool(const DescriptorSetLayout& layout) :
        _pool(create_descriptor_pool(layout, pool_size)),
        _layout(layout.vk_descriptor_set_layout()) {

    std::array<VkDescriptorSetLayout, pool_size> layouts;
    std::fill_n(layouts.begin(), pool_size, _layout);

    VkDescriptorSetAllocateInfo allocate_info = vk_struct();
    {
        allocate_info.descriptorPool = _pool;
        allocate_info.descriptorSetCount = pool_size;
        allocate_info.pSetLayouts = layouts.data();
    }

    vk_check(vkAllocateDescriptorSets(vk_device(), &allocate_info, _sets.data()));

    y_debug_assert(used_sets() == 0);
}

DescriptorSetPool::~DescriptorSetPool() {
    y_debug_assert(used_sets() == 0);
    destroy_graphic_resource(std::move(_pool));
}

void DescriptorSetPool::update_set(u32 id, core::Span<Descriptor> descriptors) {
    y_profile();

    const usize descriptor_count = descriptors.size();
    core::ScratchPad<VkWriteDescriptorSetInlineUniformBlock> inline_blocks(descriptor_count);
    core::ScratchPad<VkWriteDescriptorSetAccelerationStructureKHR> accel_structs(descriptor_count);
    core::ScratchPad<VkWriteDescriptorSet> writes(descriptor_count);

    for(usize i = 0; i != descriptor_count; ++i) {
        auto& write = writes[i];
        descriptors[i].fill_write(u32(i), write, inline_blocks[i], accel_structs[i]);
        writes[i].dstSet = _sets[id];
    }

    vkUpdateDescriptorSets(vk_device(), u32(writes.size()), writes.data(), 0, nullptr);
}

DescriptorSetData DescriptorSetPool::alloc(core::Span<Descriptor> descriptors) {
    y_profile();

    const auto lock = std::unique_lock(_lock);

    u32 id = u32(-1);
    for(usize i = 0; i != _taken.size(); ++i) {
        y_debug_assert(i == 0 || _taken[i - 1] == u64(-1));
        const u64 ones = std::countr_one(_taken[i]);
        if(ones < 64) {
            id = u32(i * 64) + u32(ones);
            y_debug_assert(!is_taken(id));

            const u64 mask = u64(1) << ones;
            _taken[i] = _taken[i] | mask;

            break;
        }
    }

    y_always_assert(id < pool_size, "DescriptorSetPool is full");
    y_debug_assert(is_taken(id));

    update_set(id, descriptors);
    return DescriptorSetData(this, id);
}

void DescriptorSetPool::recycle(u32 id) {
    y_profile();

    const auto lock = std::unique_lock(_lock);
    y_debug_assert(is_taken(id));

    const u32 index = id / 64;
    const u64 mask = u64(1) << (id % 64);
    _taken[index] = _taken[index] & ~mask;
}

bool DescriptorSetPool::is_full() const {
    for(const u64 bits : _taken) {
        if(bits != u64(-1)) {
            return false;
        }
    }
    return true;
}

VkDescriptorSet DescriptorSetPool::vk_descriptor_set(u32 id) const {
    return _sets[id];
}

VkDescriptorPool DescriptorSetPool::vk_pool() const {
    return _pool;
}

VkDescriptorSetLayout DescriptorSetPool::vk_descriptor_set_layout() const {
    return _layout;
}


bool DescriptorSetPool::is_taken(u32 id) const {
    y_debug_assert(id < pool_size);
    const u32 index = id / 64;
    const u64 mask = u64(1) << (id % 64);
    return (_taken[index] & mask) == mask;
}

usize DescriptorSetPool::free_sets() const {
    return pool_size - used_sets();
}

usize DescriptorSetPool::used_sets() const {
    const auto lock = std::unique_lock(_lock);

    usize used = 0;
    for(const u64 bits : _taken) {
        used += std::popcount(bits);
    }
    return used;
}



DescriptorSetAllocator::DescriptorSetAllocator() {
}


DescriptorSetData DescriptorSetAllocator::create_descritptor_set(core::Span<Descriptor> descriptors) {
    y_profile();

    core::ScratchPad<VkDescriptorSetLayoutBinding> layout_bindings(descriptors.size());
    for(usize i = 0; i != descriptors.size(); ++i) {
        layout_bindings[i] = descriptors[i].descriptor_set_layout_binding(u32(i));
    }

    LayoutPools* layout_p = _layouts.locked([&](auto&& layouts) {
        Y_TODO(get rid of layout binding stuff, we shouldnt need the extra alloc)
        return layout_pool(layouts, layout_bindings);
    });

    return layout_p->pools.locked([&](auto&& pools) {
        for(auto& page : core::reversed(pools)) {
            if(!page->is_full()) {
                return page->alloc(descriptors);
            }
        }

        pools.emplace_back(std::make_unique<DescriptorSetPool>(layout_p->layout));
        return pools.last()->alloc(descriptors);
    });
}

const DescriptorSetLayout& DescriptorSetAllocator::descriptor_set_layout(LayoutKey bindings) {
    return _layouts.locked([&](auto&& layouts) -> const LayoutPools* { return layout_pool(layouts, bindings); })->layout;
}

DescriptorSetAllocator::LayoutPools* DescriptorSetAllocator::layout_pool(LayoutMap& layouts, LayoutKey bindings) {
    auto& layout  = layouts[bindings];
    if(!layout) {
        layout = std::make_unique<LayoutPools>(bindings);
    }
    return layout.get();
}

usize DescriptorSetAllocator::layout_count() const {
    return _layouts.locked([](auto&& layouts) { return layouts.size(); });
}

usize DescriptorSetAllocator::pool_count() const {
    return _layouts.locked([](auto&& layouts) {
        usize count = 0;
        for(const auto& l : layouts) {
            count += l.second->pools.locked([](auto&& pools) { return pools.size(); });
        }
        return count;
    });
}

usize DescriptorSetAllocator::free_sets() const {
    return _layouts.locked([](auto&& layouts) {
        usize count = 0;
        for(const auto& l : layouts) {
            l.second->pools.locked([&](auto&& pools) {
                for(const auto& p : pools) {
                    count += p->free_sets();
                }
            });
        }
        return count;
    });
}

usize DescriptorSetAllocator::used_sets() const {
    return _layouts.locked([](auto&& layouts) {
        usize count = 0;
        for(const auto& l : layouts) {
            l.second->pools.locked([&](auto&& pools) {
                for(const auto& p : pools) {
                    count += p->used_sets();
                }
            });
        }
        return count;
    });
}

}

