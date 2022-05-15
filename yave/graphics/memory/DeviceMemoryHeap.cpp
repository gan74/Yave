/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#include "DeviceMemoryHeap.h"
#include "alloc.h"

#include <numeric>
#include <mutex>

namespace yave {

static u64 align_size(u64 total_byte_size, u64 alignent) {
    return (total_byte_size + alignent - 1) & ~(alignent - 1);
}

DeviceMemoryHeap::DeviceMemoryHeap(u32 type_bits, MemoryType type, u64 heap_size) :
        _memory(alloc_memory(heap_size, type_bits, type)),
        _heap_size(heap_size),
        _mapping(nullptr) {

    _free_blocks << FreeBlock { 0, heap_size };

    if(is_cpu_visible(type)) {
        const VkMemoryMapFlags flags = {};
        vk_check(vkMapMemory(vk_device(), _memory, 0, heap_size, flags, &_mapping));
    }

    if(_heap_size % alignment) {
        y_fatal("Heap size is not a multiple of alignment.");
    }
}

DeviceMemoryHeap::~DeviceMemoryHeap() {
    const auto lock = y_profile_unique_lock(_lock);

    sort_and_compact_blocks();

    y_always_assert(_free_blocks.size() == 1, "Not all memory has been released: heap fragmented");
    y_always_assert(_free_blocks[0].offset == 0 && _free_blocks[0].size == _heap_size, "Not all memory has been freed");

    if(_mapping) {
        vkUnmapMemory(vk_device(), _memory);
    }
    vkFreeMemory(vk_device(), _memory, vk_allocation_callbacks());
}

DeviceMemory DeviceMemoryHeap::create(u64 offset, u64 size) {
    y_profile();
    return DeviceMemory(this, _memory, offset, size);
}

core::Result<DeviceMemory> DeviceMemoryHeap::alloc(VkMemoryRequirements reqs) {
    y_profile();
    const u64 size = align_size(reqs.size, alignment);

    const auto lock = y_profile_unique_lock(_lock);

    sort_and_compact_blocks();

    for(auto it = _free_blocks.begin(); it != _free_blocks.end(); ++it) {
        const u64 full_size = it->size;

        if(full_size < size) {
            continue;
        }

        const u64 offset = it->offset;

        const u64 aligned_offset = align_size(offset, reqs.alignment);
        const u64 align_correction = aligned_offset - offset;
        const u64 aligned_size = full_size - align_correction;

        y_debug_assert(aligned_size % alignment == 0);
        y_debug_assert(aligned_offset % alignment == 0);
        y_debug_assert(aligned_offset % reqs.alignment == 0);

        if(aligned_size == size) {
            _free_blocks.erase_unordered(it);
            if(align_correction) {
                _free_blocks << FreeBlock{offset, align_correction};
            }
            return core::Ok(create(aligned_offset, size));
        } else if(aligned_size > size) {
            const u64 end = offset + full_size;
            it->size = end - (it->offset = aligned_offset + size);
            if(align_correction) {
                _free_blocks << FreeBlock{offset, align_correction};
            }
            return core::Ok(create(aligned_offset, size));
        }
    }

    return core::Err();
}

void DeviceMemoryHeap::free(const DeviceMemory& memory) {
    y_debug_assert(memory.vk_memory() == _memory);

    const auto lock = y_profile_unique_lock(_lock);

    _free_blocks << FreeBlock {
        memory.vk_offset(),
        memory.vk_size()
    };

    _should_compact = true;
}


void DeviceMemoryHeap::sort_and_compact_blocks() {
    y_profile();

    y_debug_assert(!_lock.try_lock());

    const usize block_count = _free_blocks.size();
    if(!_should_compact || block_count < 2) {
        return;
    }

    _should_compact = false;

    std::sort(_free_blocks.begin(), _free_blocks.end(), [](const auto& a, const auto& b) {
        return a.offset < b.offset;
    });

    usize dst = 0;
    for(usize i = 1; i != block_count; ++i) {
        const u64 dst_end = _free_blocks[dst].offset + _free_blocks[dst].size;
        if(_free_blocks[i].offset == dst_end) {
            _free_blocks[dst].size += _free_blocks[i].size;
        } else {
            y_debug_assert(_free_blocks[i].offset > dst_end);
            ++dst;
            _free_blocks[dst] = _free_blocks[i];
        }
    }

    while(_free_blocks.size() != dst + 1) {
          _free_blocks.pop();
    }
}

void* DeviceMemoryHeap::map(const DeviceMemoryView& view) {
    return static_cast<u8*>(_mapping) + view.vk_offset();
}

void DeviceMemoryHeap::unmap(const DeviceMemoryView&) {
}

u64 DeviceMemoryHeap::size() const {
    return _heap_size;
}

u64 DeviceMemoryHeap::available() const {
    const auto lock = y_profile_unique_lock(_lock);
    u64 tot = 0;
    for(const auto& b : _free_blocks) {
        tot += b.size;
    }
    return tot;
}

usize DeviceMemoryHeap::free_blocks() const {
    const auto lock = y_profile_unique_lock(_lock);
    return _free_blocks.size();
}

}

