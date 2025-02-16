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
#ifndef YAVE_GRAPHICS_MEMORY_DEVICEMEMORYHEAPBASE_H
#define YAVE_GRAPHICS_MEMORY_DEVICEMEMORYHEAPBASE_H

#include <yave/graphics/graphics.h>

#include <y/core/Result.h>

#include "MemoryType.h"
#include "DeviceMemory.h"
#include "DeviceMemoryView.h"

namespace yave {

class DeviceMemoryHeapBase : NonMovable {
    public:
        virtual ~DeviceMemoryHeapBase() {
        }

        virtual core::Result<DeviceMemory> alloc(VkMemoryRequirements reqs) = 0;
        virtual void free(const DeviceMemory& memory) = 0;

        virtual void* map(const VkMappedMemoryRange& range, MappingAccess access) = 0;
        virtual void unmap(const VkMappedMemoryRange& range, MappingAccess access) = 0;

        MemoryType memory_type() const;

    protected:
        DeviceMemoryHeapBase(MemoryType type);

        void invalidate_for_map(const VkMappedMemoryRange& range, MappingAccess access);
        void flush_for_unmap(const VkMappedMemoryRange& range, MappingAccess access);

        const MemoryType _type;
};

}

#endif // YAVE_GRAPHICS_MEMORY_DEVICEMEMORYHEAPBASE_H

