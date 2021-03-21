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

#include "SubBuffer.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/device/DeviceProperties.h>

namespace yave {

SubBufferBase::SubBufferBase(const BufferBase& base, usize byte_len, usize byte_off) :
        _size(byte_len),
        _offset(byte_off),
        _buffer(base.vk_buffer()),
        _memory(base.device_memory()) {

    y_debug_assert(base.byte_size() >= _size + _offset);
}

SubBufferBase::SubBufferBase(const BufferBase& base) : SubBufferBase(base, base.byte_size(), 0) {
}

bool SubBufferBase::is_null() const {
    return !_buffer;
}

usize SubBufferBase::alignment_for_usage(BufferUsage usage) {
    const auto& props = device_properties();
    u64 align = props.non_coherent_atom_size;
    if((usage & BufferUsage::UniformBit) != BufferUsage::None) {
        align = std::max(props.uniform_buffer_alignment, align);
    }
    if((usage & BufferUsage::StorageBit) != BufferUsage::None) {
        align = std::max(props.storage_buffer_alignment, align);
    }
    return usize(align);
}

usize SubBufferBase::byte_size() const {
    return _size;
}

usize SubBufferBase::byte_offset() const {
    return _offset;
}

VkBuffer SubBufferBase::vk_buffer() const {
    return _buffer;
}

DeviceMemoryView SubBufferBase::device_memory() const {
    return _memory;
}

VkDescriptorBufferInfo SubBufferBase::descriptor_info() const {
    VkDescriptorBufferInfo info = {};
    {
        info.buffer = _buffer;
        info.offset = _offset;
        info.range = _size;
    }
    return info;
}

VkMappedMemoryRange SubBufferBase::vk_memory_range() const {
    return _memory.vk_mapped_range(_size, _offset);
}

}

