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

#include "DeviceMemoryHeapBase.h"

namespace yave {

DeviceMemoryHeapBase::DeviceMemoryHeapBase(MemoryType type) : _type(type) {

}

void DeviceMemoryHeapBase::invalidate_for_map(const VkMappedMemoryRange& range, MappingAccess access) {
    switch(access) {
        case MappingAccess::ReadOnly:
        case MappingAccess::ReadWrite:
            vk_check(vkInvalidateMappedMemoryRanges(vk_device(), 1, &range));
        break;

        case MappingAccess::WriteOnly:
        break;
    }
}

void DeviceMemoryHeapBase::flush_for_unmap(const VkMappedMemoryRange& range, MappingAccess access) {
    switch(access) {
        case MappingAccess::WriteOnly:
        case MappingAccess::ReadWrite:
            vk_check(vkFlushMappedMemoryRanges(vk_device(), 1, &range));
        break;

        case MappingAccess::ReadOnly:
        break;
    }
}

MemoryType DeviceMemoryHeapBase::memory_type() const {
    return _type;
}

}

