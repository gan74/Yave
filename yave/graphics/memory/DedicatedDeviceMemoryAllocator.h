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
#ifndef YAVE_GRAPHICS_MEMORY_DEDICATEDDEVICEMEMORYALLOCATOR_H
#define YAVE_GRAPHICS_MEMORY_DEDICATEDDEVICEMEMORYALLOCATOR_H

#include "DeviceMemoryHeapBase.h"

#include <y/core/HashMap.h>
#include <y/concurrent/Mutexed.h>

namespace yave {

// Allocator disgased as a heap
class DedicatedDeviceMemoryAllocator : public DeviceMemoryHeapBase {
    public:
        DedicatedDeviceMemoryAllocator(MemoryType type);

        ~DedicatedDeviceMemoryAllocator() override;

        core::Result<DeviceMemory> alloc(VkMemoryRequirements reqs) override;
        void free(const DeviceMemory& memory) override;

        void* map(const VkMappedMemoryRange& range, MappingAccess access) override;
        void unmap(const VkMappedMemoryRange& range, MappingAccess access) override;

        u64 allocated_size() const;
        usize allocation_count() const;

    private:
        std::atomic<u64> _total_size = 0;
        std::atomic<usize> _alloc_count = 0;

        concurrent::Mutexed<core::FlatHashMap<VkDeviceMemory, void*>> _mappings;
};

}

#endif // YAVE_GRAPHICS_MEMORY_DEDICATEDDEVICEMEMORYALLOCATOR_H

