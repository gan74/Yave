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

#include "DeviceMemoryAllocator.h"

#include <yave/graphics/device/DeviceProperties.h>

#include <y/utils/log.h>

namespace yave {

Y_TODO(DeviceAllocator should track allocation count)


u64 DeviceMemoryAllocator::heap_size_for_type(MemoryType type) {
    if(type == MemoryType::Staging) {
        return default_heap_size / 2;
    }
    return default_heap_size;
}

DeviceMemoryAllocator::DeviceMemoryAllocator(const DeviceProperties& properties) : _max_allocs(properties.max_memory_allocations) {
}

DeviceMemory DeviceMemoryAllocator::dedicated_alloc(VkMemoryRequirements reqs, MemoryType type) {
    y_profile();
    auto& heap = _dedicated_heaps[type];
    if(!heap) {
        heap = std::make_unique<DedicatedDeviceMemoryAllocator>(type);
    }
    return std::move(heap->alloc(reqs).unwrap());
}

DeviceMemory DeviceMemoryAllocator::alloc(VkMemoryRequirements2 reqs, MemoryType type, MemoryAllocFlags flags) {
    y_profile();

    const VkMemoryRequirements& mem_reqs = reqs.memoryRequirements;
    const VkMemoryDedicatedRequirements* dedicated_reqs = vk_find_pnext<VkMemoryDedicatedRequirements>(reqs);

    const bool dedicated_alloc_allowed = (flags & MemoryAllocFlags::NoDedicatedAllocBit) == MemoryAllocFlags::None;
    const bool use_dedicated_alloc =
        (mem_reqs.size >= heap_size_for_type(type)) ||
        (dedicated_reqs && dedicated_reqs->prefersDedicatedAllocation && dedicated_alloc_allowed);

    Y_TODO(We are double locking here, each heap will lock internally)
    const auto lock = std::unique_lock(_lock);

    if(use_dedicated_alloc) {
        return dedicated_alloc(mem_reqs, type);
    }

    if(mem_reqs.alignment % DeviceMemoryHeap::alignment && DeviceMemoryHeap::alignment % mem_reqs.alignment) {
        log_msg("Failed to align memory: defaulting to dedicated allocation", Log::Warning);
        return dedicated_alloc(mem_reqs, type);
    }

    auto& heaps =_heaps[HeapType{mem_reqs.memoryTypeBits, type}];
    for(auto& heap : heaps) {
        if(auto r = heap->alloc(mem_reqs)) {
            return std::move(r.unwrap());
        }
    }

    auto heap = std::make_unique<DeviceMemoryHeap>(mem_reqs.memoryTypeBits, type, heap_size_for_type(type));
    auto alloc = std::move(heap->alloc(mem_reqs).unwrap());

    heaps.push_back(std::move(heap));

    return alloc;
}

DeviceMemory DeviceMemoryAllocator::alloc(VkImage image, MemoryAllocFlags flags) {
    VkImageMemoryRequirementsInfo2 infos = vk_struct();
    infos.image = image;

    VkMemoryDedicatedRequirements dedicated = vk_struct();
    VkMemoryRequirements2 reqs = vk_struct();
    reqs.pNext = &dedicated;

    vkGetImageMemoryRequirements2(vk_device(), &infos, &reqs);
    return alloc(reqs, MemoryType::DeviceLocal, flags);
}

DeviceMemory DeviceMemoryAllocator::alloc(VkBuffer buffer, MemoryType type, MemoryAllocFlags flags) {
    VkBufferMemoryRequirementsInfo2 infos = vk_struct();
    infos.buffer = buffer;

    VkMemoryDedicatedRequirements dedicated = vk_struct();
    VkMemoryRequirements2 reqs = vk_struct();
    reqs.pNext = &dedicated;

    vkGetBufferMemoryRequirements2(vk_device(), &infos, &reqs);
    return alloc(reqs, type, flags);
}

}

