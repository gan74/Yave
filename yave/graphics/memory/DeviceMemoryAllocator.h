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
#ifndef YAVE_GRAPHICS_MEMORY_DEVICEMEMORYALLOCATOR_H
#define YAVE_GRAPHICS_MEMORY_DEVICEMEMORYALLOCATOR_H

#include "DeviceMemoryHeap.h"
#include "DedicatedDeviceMemoryAllocator.h"

#include <y/utils/hash.h>
#include <y/core/Range.h>
#include <y/core/Vector.h>
#include <y/core/HashMap.h>

#include <memory>

namespace yave {

class DeviceMemoryAllocator {

    using HeapType = std::pair<u32, MemoryType>;

    static constexpr usize default_heap_size = 128 * 1024 * 1024;

    public:
        DeviceMemoryAllocator();

        DeviceMemory alloc(VkImage image);
        DeviceMemory alloc(VkBuffer buffer, MemoryType type);
        DeviceMemory alloc(VkMemoryRequirements reqs, MemoryType type);

        auto heaps() const {
            return core::Range(_heaps.begin(), _heaps.end());
        }

        auto dedicated_heaps() const {
            return core::Range(_dedicated_heaps.begin(), _dedicated_heaps.end());
        }

    private:
        static usize heap_size_for_type(MemoryType type);
        static usize dedicated_threshold_for_type(MemoryType type);

        DeviceMemory dedicated_alloc(VkMemoryRequirements reqs, MemoryType type);

        core::ExternalHashMap<HeapType, core::Vector<std::unique_ptr<DeviceMemoryHeap>>> _heaps;
        core::ExternalHashMap<MemoryType, std::unique_ptr<DedicatedDeviceMemoryAllocator>> _dedicated_heaps;

        usize _max_allocs = 0;
        mutable std::mutex _lock;
};

}

#endif // YAVE_GRAPHICS_MEMORY_DEVICEMEMORYALLOCATOR_H

