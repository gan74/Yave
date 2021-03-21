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

#include "DescriptorSetAllocator.h"
#include "Descriptor.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/device/DeviceProperties.h>

#include <y/core/Range.h>
#include <y/utils/log.h>
#include <y/utils/format.h>

namespace yave {

static constexpr usize inline_block_index = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT + 1;

static core::Vector<VkDescriptorSetLayoutBinding> create_layout_bindings(core::Span<Descriptor> descriptors) {
    auto layout_bindings = core::vector_with_capacity<VkDescriptorSetLayoutBinding>(descriptors.size());
    for(const Descriptor& d : descriptors) {
        layout_bindings << d.descriptor_set_layout_binding(u32(layout_bindings.size()));
    }
    return layout_bindings;
}


static usize descriptor_type_index(VkDescriptorType type) {
    if(type == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT) {
        y_debug_assert(inline_block_index < DescriptorSetLayout::descriptor_type_count);
        return inline_block_index;
    }

    y_debug_assert(usize(type) < DescriptorSetLayout::descriptor_type_count);
    return usize(type);
}

static VkDescriptorType index_descriptor_type(usize index) {
    y_debug_assert(index <  DescriptorSetLayout::descriptor_type_count);
    if(index == inline_block_index) {
        return VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT;
    }
    return VkDescriptorType(index);
}

static VkDescriptorSetLayoutBinding create_inline_uniform_binding_fallback(const VkDescriptorSetLayoutBinding& binding) {
    if(binding.descriptorType != VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT) {
        return binding;
    }

    VkDescriptorSetLayoutBinding fallback = binding;
    fallback.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    fallback.descriptorCount = 1;

    return fallback;
}



DescriptorSetLayout::DescriptorSetLayout(core::Span<VkDescriptorSetLayoutBinding> bindings) {
    const usize max_inline_uniform_size = device_properties().max_inline_uniform_size;
    const bool inline_uniform_supported = max_inline_uniform_size != 0;
    const auto needs_fallback = [=](const VkDescriptorSetLayoutBinding& binding) {
        return binding.descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT && (!inline_uniform_supported || binding.descriptorCount > max_inline_uniform_size);
    };

    core::Vector<VkDescriptorSetLayoutBinding> patched_bindings;
    if(std::any_of(bindings.begin(), bindings.end(), needs_fallback)) {
        patched_bindings.set_min_capacity(bindings.size());
        for(const auto& b : bindings) {
            if(needs_fallback(b)) {
                log_msg(fmt("Inline uniform at binding % of size % requires fallback uniform buffer", b.binding, b.descriptorCount), Log::Warning);
                patched_bindings << create_inline_uniform_binding_fallback(b);
                _inline_blocks_fallbacks << InlineBlock{b.binding, b.descriptorCount};
            } else {
                patched_bindings << b;
            }
        }
        bindings = patched_bindings;
    }

    for(const auto& b : bindings) {
        if(b.descriptorType == VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK_EXT) {
            ++_inline_blocks;
        }
        _sizes[descriptor_type_index(b.descriptorType)] += b.descriptorCount;
    }

    VkDescriptorSetLayoutCreateInfo create_info = vk_struct();
    {
        create_info.bindingCount = u32(bindings.size());
        create_info.pBindings = bindings.data();
    }
    vk_check(vkCreateDescriptorSetLayout(vk_device(), &create_info, vk_allocation_callbacks(), &_layout.get()));
}

DescriptorSetLayout::~DescriptorSetLayout() {
    device_destroy(_layout);
}

bool DescriptorSetLayout::is_null() const {
    return !_layout;
}

const std::array<u32, DescriptorSetLayout::descriptor_type_count>& DescriptorSetLayout::desciptors_count() const {
    return _sizes;
}

core::Span<DescriptorSetLayout::InlineBlock> DescriptorSetLayout::inline_blocks_fallbacks() const {
    return _inline_blocks_fallbacks;
}

usize DescriptorSetLayout::inline_blocks() const {
    return _inline_blocks;
}

VkDescriptorSetLayout DescriptorSetLayout::vk_descriptor_set_layout() const {
    return _layout;
}



static VkDescriptorPool create_descriptor_pool(const DescriptorSetLayout& layout, usize set_count) {
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

    VkDescriptorPool pool = {};
    vk_check(vkCreateDescriptorPool(vk_device(), &create_info, vk_allocation_callbacks(), &pool));
    return pool;
}

DescriptorSetPool::DescriptorSetPool(const DescriptorSetLayout& layout) :
    _pool(create_descriptor_pool(layout, pool_size)),
    _layout(layout.vk_descriptor_set_layout()),
    _inline_blocks(layout.inline_blocks()) {

    std::array<VkDescriptorSetLayout, pool_size> layouts;
    std::fill_n(layouts.begin(), pool_size, layout.vk_descriptor_set_layout());

    VkDescriptorSetAllocateInfo allocate_info = vk_struct();
    {
        allocate_info.descriptorPool = _pool;
        allocate_info.descriptorSetCount = pool_size;
        allocate_info.pSetLayouts = layouts.data();
    }

    vk_check(vkAllocateDescriptorSets(vk_device(), &allocate_info, _sets.data()));

    if(!layout.inline_blocks_fallbacks().is_empty()) {
        const usize alignment = inline_sub_buffer_alignment();
        for(const auto& buffer : layout.inline_blocks_fallbacks()) {
            _descriptor_buffer_size += memory::align_up_to(buffer.byte_size, alignment);
        }
        log_msg(fmt("Allocation % * % bytes of inline uniform storage", _descriptor_buffer_size, pool_size));
        _inline_buffer = Buffer<BufferUsage::UniformBit>(_descriptor_buffer_size * pool_size);
    }
}

DescriptorSetPool::~DescriptorSetPool() {
    y_debug_assert(_taken.none());
    device_destroy(_pool);
}

usize DescriptorSetPool::inline_sub_buffer_alignment() const {
    return SubBuffer<BufferUsage::UniformBit>::alignment();
}

DescriptorSetData DescriptorSetPool::alloc(core::Span<Descriptor> descriptors) {
    y_profile();

    const auto lock = y_profile_unique_lock(_lock);
    if(is_full() || _taken[_first_free]) {
        y_fatal("DescriptorSetPoolPage is full.");
    }
    const u32 id = _first_free;

    for(++_first_free; _first_free < pool_size; ++_first_free) {
        if(!_taken[_first_free]) {
            break;
        }
    }

    y_debug_assert(is_full() || !_taken[_first_free]);

    _taken.set(id);

    update_set(id, descriptors);
    return DescriptorSetData(this, id);
}

void DescriptorSetPool::update_set(u32 id, core::Span<Descriptor> descriptors) {
    const usize max_inline_uniform_size = device_properties().max_inline_uniform_size;
    const bool inline_uniform_supported = max_inline_uniform_size != 0;
    const usize block_buffer_alignment = inline_sub_buffer_alignment();

    core::Vector<VkWriteDescriptorSetInlineUniformBlockEXT> inline_blocks;
    core::Vector<VkDescriptorBufferInfo> inline_blocks_buffer_infos;

    if(_inline_blocks) {
        inline_blocks.set_min_capacity(descriptors.size());
    }
    if(!_inline_buffer.is_null()) {
        inline_blocks_buffer_infos.set_min_capacity(descriptors.size());
    }

    usize inline_buffer_offset = 0;
    auto writes = core::vector_with_capacity<VkWriteDescriptorSet>(descriptors.size());
    for(const auto& desc : descriptors) {
        const u32 descriptor_count = desc.descriptor_set_layout_binding(0).descriptorCount;
        VkWriteDescriptorSet write = vk_struct();
        {
            write.dstSet = _sets[id];
            write.dstBinding = u32(writes.size());
            write.dstArrayElement = 0;
            write.descriptorCount = descriptor_count;
            write.descriptorType = desc.vk_descriptor_type();
        }

        if(desc.is_buffer()) {
            write.pBufferInfo = &desc.descriptor_info().buffer;
        } else if(desc.is_image()) {
            write.pImageInfo = &desc.descriptor_info().image;
        } else if(desc.is_inline_block()) {

            const Descriptor::InlineBlock block = desc.descriptor_info().inline_block;
            if(!inline_uniform_supported || block.size > max_inline_uniform_size) {
                const usize aligned_block_size = memory::align_up_to(block.size, block_buffer_alignment);
                const usize buffer_start = id * _descriptor_buffer_size + inline_buffer_offset;

                SubBuffer<BufferUsage::UniformBit, MemoryType::CpuVisible> block_buffer(_inline_buffer, aligned_block_size, buffer_start);
                {
                    Mapping mapping(block_buffer);
                    std::memcpy(mapping.data(), block.data, block.size);
                }

                write.pBufferInfo = &inline_blocks_buffer_infos.emplace_back(block_buffer.descriptor_info());
                write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                write.descriptorCount = 1;

                inline_buffer_offset += aligned_block_size;
            } else {
                VkWriteDescriptorSetInlineUniformBlockEXT& inline_block = inline_blocks.emplace_back(VkWriteDescriptorSetInlineUniformBlockEXT(vk_struct()));
                inline_block.pData = block.data;
                inline_block.dataSize = u32(block.size);
                write.pNext = &inline_block;
                y_debug_assert(inline_block.dataSize % 4 == 0);
            }

        } else {
            y_fatal("Unknown descriptor type.");
        }

        writes << write;
    }

    vkUpdateDescriptorSets(vk_device(), u32(writes.size()), writes.data(), 0, nullptr);
}


void DescriptorSetPool::recycle(u32 id) {
    y_profile();

    const auto lock = y_profile_unique_lock(_lock);
    y_debug_assert(_taken[id]);
    _taken.reset(id);
    _first_free = std::min(_first_free, id);
}

bool DescriptorSetPool::is_full() const {
    // we shouldn't need to lock here
    return _first_free >= pool_size;
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

usize DescriptorSetPool::free_sets() const {
    return pool_size - used_sets();
}

usize DescriptorSetPool::used_sets() const {
    const auto lock = y_profile_unique_lock(_lock);
    return _taken.count();
}



DescriptorSetAllocator::DescriptorSetAllocator() {
}


DescriptorSetData DescriptorSetAllocator::create_descritptor_set(core::Span<Descriptor> descriptors) {
    const auto lock = y_profile_unique_lock(_lock);

    Y_TODO(get rid of layout binding stuff, we shouldnt need the extra alloc)
    const auto bindings = create_layout_bindings(descriptors);
    auto& pool = layout(bindings);

    const auto reversed = core::Range(std::make_reverse_iterator(pool.pools.end()),
                                      std::make_reverse_iterator(pool.pools.begin()));
    for(auto& page : reversed) {
        if(!page->is_full()) {
            return page->alloc(descriptors);
        }
    }

    pool.pools.emplace_back(std::make_unique<DescriptorSetPool>(pool.layout));
    return pool.pools.last()->alloc(descriptors);
}

const DescriptorSetLayout& DescriptorSetAllocator::descriptor_set_layout(const Key& bindings) {
    const auto lock = y_profile_unique_lock(_lock);
    return layout(bindings).layout;
}

DescriptorSetAllocator::LayoutPools& DescriptorSetAllocator::layout(const Key& bindings) {
    auto& layout  = _layouts[bindings];
    if(layout.layout.is_null()) {
        layout.layout = DescriptorSetLayout(bindings);
    }
    return layout;
}

usize DescriptorSetAllocator::layout_count() const {
    const auto lock = y_profile_unique_lock(_lock);
    return _layouts.size();
}

usize DescriptorSetAllocator::pool_count() const {
    const auto lock = y_profile_unique_lock(_lock);
    usize count = 0;
    for(const auto& l : _layouts) {
        count += l.second.pools.size();
    }
    return count;
}

usize DescriptorSetAllocator::free_sets() const {
    const auto lock = y_profile_unique_lock(_lock);
    usize count = 0;
    for(const auto& l : _layouts) {
        for(const auto& p : l.second.pools) {
            count += p->free_sets();
        }
    }
    return count;
}

usize DescriptorSetAllocator::used_sets() const {
    const auto lock = y_profile_unique_lock(_lock);
    usize count = 0;
    for(const auto& l : _layouts) {
        for(const auto& p : l.second.pools) {
            count += p->used_sets();
        }
    }
    return count;
}

}

