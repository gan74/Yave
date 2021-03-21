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
#ifndef YAVE_GRAPHICS_MEMORY_DEVICEMEMORYHEAP_H
#define YAVE_GRAPHICS_MEMORY_DEVICEMEMORYHEAP_H

#include "DeviceMemoryHeapBase.h"

#include <y/core/Vector.h>

#include <mutex>

namespace yave {

// For DeviceAllocator, should not be used directly
class DeviceMemoryHeap : public DeviceMemoryHeapBase {

    struct FreeBlock {
        usize offset;
        usize size;

        usize end_offset() const;
        bool contiguous(const FreeBlock& blck) const;
        void merge(const FreeBlock& block);
    };

    public:
        static constexpr usize alignment = 256;


        DeviceMemoryHeap(u32 type_bits, MemoryType type, usize heap_size);
        ~DeviceMemoryHeap() override;

        core::Result<DeviceMemory> alloc(VkMemoryRequirements reqs) override;
        void free(const DeviceMemory& memory) override;

        void* map(const DeviceMemoryView& view) override;
        void unmap(const DeviceMemoryView&) override;

        usize size() const;
        usize available() const; // slow!
        usize free_blocks() const;

        bool mapped() const;

    private:
        void swap(DeviceMemoryHeap& other);

        DeviceMemory create(usize offset, usize size);
        void free(const FreeBlock& block);
        void compact_block(FreeBlock block);

        VkDeviceMemory _memory = {};
        usize _heap_size = 0;
        core::Vector<FreeBlock> _blocks;
        void* _mapping = nullptr;

        mutable std::mutex _lock;
};

}

#endif // YAVE_GRAPHICS_MEMORY_DEVICEMEMORYHEAP_H

