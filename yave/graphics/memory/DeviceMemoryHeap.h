/*******************************
Copyright (c) 2016-2024 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_MEMORY_DEVICEMEMORYHEAP_H
#define YAVE_GRAPHICS_MEMORY_DEVICEMEMORYHEAP_H

#include "DeviceMemoryHeapBase.h"

#include <y/core/Vector.h>

#include <mutex>

namespace yave {

// For DeviceAllocator, should not be used directly
class DeviceMemoryHeap : public DeviceMemoryHeapBase {

    struct FreeBlock {
        u64 offset;
        u64 size;
    };

    public:
        static constexpr u64 alignment = 256;


        DeviceMemoryHeap(u32 type_bits, MemoryType type, u64 heap_size);
        ~DeviceMemoryHeap() override;

        core::Result<DeviceMemory> alloc(VkMemoryRequirements reqs) override;
        void free(const DeviceMemory& memory) override;

        void* map(const VkMappedMemoryRange& range, MappingAccess access) override;
        void unmap(const VkMappedMemoryRange& range, MappingAccess access) override;

        u64 size() const;
        u64 available() const; // slow!
        usize free_blocks() const;

    private:
        void swap(DeviceMemoryHeap& other);

        DeviceMemory create(u64 offset, u64 size);
        void sort_and_compact_blocks();

        VkDeviceMemory _memory = {};
        u64 _heap_size = 0;
        void* _mapping = nullptr;

        core::Vector<FreeBlock> _free_blocks;
        bool _should_compact = false;
        mutable std::mutex _lock;
};

}

#endif // YAVE_GRAPHICS_MEMORY_DEVICEMEMORYHEAP_H

