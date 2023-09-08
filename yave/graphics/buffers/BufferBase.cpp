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

#include "BufferBase.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/device/DeviceProperties.h>
#include <yave/graphics/memory/DeviceMemoryAllocator.h>

#include <y/utils/format.h>

namespace yave {

static void bind_buffer_memory(VkBuffer buffer, const DeviceMemory& memory) {
    vk_check(vkBindBufferMemory(vk_device(), buffer, memory.vk_memory(), memory.vk_offset()));
}

static VkBuffer create_buffer(u64 byte_size, VkBufferUsageFlags usage) {
    y_debug_assert(byte_size);
    if(usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
        if(byte_size > device_properties().max_uniform_buffer_size) {
            y_fatal("Uniform buffer size exceeds maxUniformBufferRange ({}).", device_properties().max_uniform_buffer_size);
        }
    }

    VkBufferCreateInfo create_info = vk_struct();
    {
        create_info.size = byte_size;
        create_info.usage = usage;
        create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VkBuffer buffer = {};
    vk_check(vkCreateBuffer(vk_device(), &create_info, vk_allocation_callbacks(), &buffer));
    return buffer;
}

static std::tuple<VkBuffer, DeviceMemory> alloc_buffer(u64 buffer_size, VkBufferUsageFlags usage, MemoryType type) {
    y_profile();

    y_debug_assert(buffer_size);

    const auto buffer = create_buffer(buffer_size, usage);
    auto memory = device_allocator().alloc(buffer, type);
    bind_buffer_memory(buffer, memory);

    return {buffer, std::move(memory)};
}




SubBufferBase::SubBufferBase(const BufferBase& base, u64 byte_len, u64 byte_off) :
        _size(byte_len),
        _offset(byte_off),
        _buffer(base.vk_buffer()),
        _memory(base.device_memory()) {

    y_debug_assert(base.byte_size() >= byte_len + byte_off);
}

SubBufferBase::SubBufferBase(const SubBufferBase& base, u64 byte_len, u64 byte_off) :
        _size(byte_len),
        _offset(byte_off + base.byte_offset()),
        _buffer(base.vk_buffer()),
        _memory(base.device_memory()) {

    y_debug_assert(base.byte_size() >= byte_len + byte_off);
}

SubBufferBase::SubBufferBase(const BufferBase& base) : SubBufferBase(base, base.byte_size(), 0) {
}

bool SubBufferBase::is_null() const {
    return !_buffer;
}

u64 SubBufferBase::host_side_alignment() {
    return device_properties().non_coherent_atom_size;
}

u64 SubBufferBase::alignment_for_usage(BufferUsage usage) {
    u64 align = 1;
    const auto& props = device_properties();
    if((usage & BufferUsage::UniformBit) != BufferUsage::None) {
        align = std::max(props.uniform_buffer_alignment, align);
    }
    if((usage & BufferUsage::StorageBit) != BufferUsage::None) {
        align = std::max(props.storage_buffer_alignment, align);
    }
    return u64(align);
}

u64 SubBufferBase::byte_size() const {
    return _size;
}

u64 SubBufferBase::byte_offset() const {
    return _offset;
}

VkBuffer SubBufferBase::vk_buffer() const {
    return _buffer;
}

DeviceMemoryView SubBufferBase::device_memory() const {
    return _memory;
}

VkDescriptorBufferInfo SubBufferBase::vk_descriptor_info() const {
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


bool SubBufferBase::operator==(const SubBufferBase& other) const {
    return (_buffer == other._buffer) && (_offset == other._offset) && (_size == other._size);
}

bool SubBufferBase::operator!=(const SubBufferBase& other) const {
    return !operator==(other);
}


BufferBase::BufferBase(u64 byte_size, BufferUsage usage, MemoryType type) : _size(byte_size), _usage(usage) {
    std::tie(*_buffer.get_ptr_for_init(), _memory) = alloc_buffer(byte_size, VkBufferUsageFlagBits(usage), type);
}

BufferBase::~BufferBase() {
    destroy_graphic_resource(std::move(_buffer));
    destroy_graphic_resource(std::move(_memory));
}

BufferUsage BufferBase::usage() const {
    return _usage;
}

u64 BufferBase::byte_size() const {
    return _size;
}

bool BufferBase::is_null() const {
    return !_buffer;
}

VkBuffer BufferBase::vk_buffer() const {
    return _buffer;
}

const DeviceMemory& BufferBase::device_memory() const {
    return _memory;
}

VkDescriptorBufferInfo BufferBase::vk_descriptor_info() const {
    VkDescriptorBufferInfo info = {};
    {
        info.buffer = _buffer;
        info.offset = 0;
        info.range = byte_size();
    }
    return info;

}

}

