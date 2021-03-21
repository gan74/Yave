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
#ifndef YAVE_GRAPHICS_MEMORY_ALLOC_H
#define YAVE_GRAPHICS_MEMORY_ALLOC_H

#include <yave/graphics/graphics.h>
#include <yave/graphics/device/PhysicalDevice.h>

#include "MemoryType.h"

#include <y/utils/format.h>

// THIS FILE SHOULD NOT BE INCLUDED OUTSIDE OF MEMORY'S CPPs !!!

namespace yave {

static const VkMemoryPropertyFlags dont_care_flags[] = {
    0
};

static const VkMemoryPropertyFlags device_local_flags[] = {
    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    0
};

static const VkMemoryPropertyFlags cpu_visible_flags[] = {
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    0
};

static const VkMemoryPropertyFlags staging_flags[] = {
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
    VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    0
};



static const VkMemoryPropertyFlags* memory_type_flags[] = {
    dont_care_flags,    // DontCare
    device_local_flags, // DeviceLocal
    cpu_visible_flags,  // CpuVisible
    staging_flags       // Staging
};



// https://en.wikipedia.org/wiki/Hamming_weight
static inline u32 popcnt_32(u32 x) {
    x -= (x >> 1) & 0x55555555;
    x = (x & 0x33333333) + ((x >> 2) & 0x33333333);
    x = (x + (x >> 4)) & 0x0f0f0f0f;
    return (x * 0x01010101) >> 24;
}

inline u32 get_memory_type(const VkPhysicalDeviceMemoryProperties& properties, u32 type_filter, MemoryType type) {
    for(const VkMemoryPropertyFlags* type_flags = memory_type_flags[uenum(type)]; *type_flags; ++type_flags) {
        const VkMemoryPropertyFlags flags = *type_flags;

        u32 best_index = u32(-1);
        u32 best_pop = u32(-1);

        for(u32 i = 0; i != properties.memoryTypeCount; ++i) {
            if((type_filter & (1 << i)) == 0) {
                continue;
            }
            const auto memory_type = properties.memoryTypes[i];
            if(memory_type.propertyFlags == flags) {
                return i;
            } else if((memory_type.propertyFlags & flags) == flags) {
                const u32 pop = popcnt_32(memory_type.propertyFlags);
                if(pop < best_pop) {
                    best_pop = pop;
                    best_index = i;
                }
            }
        }

        if(best_index != u32(-1)) {
            return best_index;
        }
    }

     y_fatal("Unable to allocate device memory.");
}



inline VkDeviceMemory alloc_memory(usize size, u32 type_bits, MemoryType type) {
    y_profile();
    VkMemoryAllocateInfo allocate_info = vk_struct();
    {
        allocate_info.allocationSize = size;
        allocate_info.memoryTypeIndex = get_memory_type(physical_device().vk_memory_properties(), type_bits, type);
    }
    VkDeviceMemory memory = {};
    const VkResult result = vkAllocateMemory(vk_device(), &allocate_info, vk_allocation_callbacks(), &memory);
    if(is_error(result)) {
        y_fatal("Failed to allocate memory: %", vk_result_str(result));
    }
    return memory;
}

inline VkDeviceMemory alloc_memory(VkMemoryRequirements reqs, MemoryType type) {
    return alloc_memory(reqs.size, reqs.memoryTypeBits, type);
}

}

#endif // YAVE_GRAPHICS_MEMORY_ALLOC_H

