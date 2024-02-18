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
#ifndef YAVE_GRAPHICS_MEMORY_DEVICEMEMORYVIEW_H
#define YAVE_GRAPHICS_MEMORY_DEVICEMEMORYVIEW_H

#include "DeviceMemory.h"

namespace yave {

enum class MappingAccess {
    WriteOnly,
    ReadOnly,
    ReadWrite
};

class DeviceMemoryView {
    public:
        DeviceMemoryView() = default;
        DeviceMemoryView(const DeviceMemory& mem);

        VkMappedMemoryRange vk_mapped_range(u64 size, u64 offset = 0) const;
        VkDeviceMemory vk_memory() const;
        u64 vk_offset() const;
        u64 vk_size() const;

        DeviceMemoryHeapBase* heap();

    private:
        DeviceMemoryHeapBase* _heap = nullptr;
        VkDeviceMemory _memory = {};
        u64 _offset = 0;
        u64 _size = 0;
};

}

#endif // YAVE_GRAPHICS_MEMORY_DEVICEMEMORYVIEW_H

