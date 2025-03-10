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

#include "BufferMapping.h"
#include "Buffer.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/commands/CmdBufferRecorder.h>

namespace yave {

BufferMappingBase::BufferMappingBase(const SubBuffer<BufferUsage::None, MemoryType::CpuVisible>& buffer, MappingAccess access) :
        _buffer(buffer),
        _access(access) {

    // vmaInvalidateAllocation ???
    // vk_check(vmaMapMemory(device_allocator(), _buffer.device_memory()._alloc, &_mapping));
    _mapping = static_cast<void*>(static_cast<u8*>(_buffer.device_memory()._mapping) + _buffer.byte_offset());

    y_debug_assert(_buffer.byte_offset() % _buffer.host_side_alignment() == 0);
    y_debug_assert(_mapping);
}

BufferMappingBase::~BufferMappingBase() {
    if(_mapping) {
#pragma message("we flush the whole alloc")
        vmaFlushAllocation(device_allocator(), _buffer.device_memory()._alloc, 0, VK_WHOLE_SIZE);
    }
}


usize BufferMappingBase::byte_size() const {
    return _buffer.byte_size();
}

void BufferMappingBase::swap(BufferMappingBase& other) {
    std::swap(_mapping, other._mapping);
    std::swap(_buffer, other._buffer);
    std::swap(_access, other._access);
}

void* BufferMappingBase::raw_data() {
    return _mapping;
}

const void* BufferMappingBase::raw_data() const {
    return _mapping;
}

}

