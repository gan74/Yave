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
#ifndef YAVE_GRAPHICS_MEMORY_ALLOC_H
#define YAVE_GRAPHICS_MEMORY_ALLOC_H

#include <yave/graphics/graphics.h>
#include <yave/graphics/device/PhysicalDevice.h>

#include "MemoryType.h"

#include <y/utils/log.h>
#include <y/utils/format.h>
#include <y/utils/memory.h>

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


inline VkDeviceMemory alloc_memory(usize size, u32 type_bits, MemoryType type) {
    y_profile();

    const VkPhysicalDeviceMemoryProperties& properties = physical_device().vk_memory_properties();
    for(const VkMemoryPropertyFlags* type_flags = memory_type_flags[uenum(type)]; *type_flags; ++type_flags) {
        const VkMemoryPropertyFlags flags = *type_flags;

        u32 best_index = u32(-1);
        u32 best_pop = u32(-1);

        for(u32 i = 0; i != properties.memoryTypeCount; ++i) {
            if((type_bits & (1 << i)) == 0) {
                continue;
            }

            if(properties.memoryHeaps[properties.memoryTypes[i].heapIndex].size <= size) {
                continue;
            }

            const VkMemoryType memory_type = properties.memoryTypes[i];
            if(memory_type.propertyFlags == flags) {
                best_index = i;
                break;
            } else if((memory_type.propertyFlags & flags) == flags) {
                const u32 pop = std::popcount(memory_type.propertyFlags);
                if(pop < best_pop) {
                    best_pop = pop;
                    best_index = i;
                }
            }
        }

        if(best_index != u32(-1)) {
            VkMemoryAllocateInfo allocate_info = vk_struct();
            {
                allocate_info.allocationSize = size;
                allocate_info.memoryTypeIndex = best_index;
            }
            VkDeviceMemory memory = {};
            const VkResult result = vkAllocateMemory(vk_device(), &allocate_info, vk_allocation_callbacks(), &memory);
            if(is_error(result)) {
                if(result == VK_ERROR_OUT_OF_DEVICE_MEMORY) {
                    log_msg(fmt("{} heap out of memory", memory_type_name(type)), Log::Warning);
                    continue;
                }
                y_fatal("Failed to allocate memory: {}", vk_result_str(result));
            }
            return memory;
        }
    }

    y_fatal("Failed to allocate memory: no suitable heap found (out of memory?)");
}

inline VkDeviceMemory alloc_memory(VkMemoryRequirements reqs, MemoryType type) {
    return alloc_memory(reqs.size, reqs.memoryTypeBits, type);
}

}

#endif // YAVE_GRAPHICS_MEMORY_ALLOC_H

