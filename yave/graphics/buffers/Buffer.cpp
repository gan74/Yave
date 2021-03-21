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

#include "Buffer.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/device/DeviceProperties.h>
#include <yave/graphics/memory/DeviceMemoryAllocator.h>

#include <y/utils/format.h>

namespace yave {

static void bind_buffer_memory(VkBuffer buffer, const DeviceMemory& memory) {
    vk_check(vkBindBufferMemory(vk_device(), buffer, memory.vk_memory(), memory.vk_offset()));
}

static VkBuffer create_buffer(usize byte_size, VkBufferUsageFlags usage) {
    y_debug_assert(byte_size);
    if(usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
        if(byte_size > device_properties().max_uniform_buffer_size) {
            y_fatal("Uniform buffer size exceeds maxUniformBufferRange (%).", device_properties().max_uniform_buffer_size);
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

static std::tuple<VkBuffer, DeviceMemory> alloc_buffer(usize buffer_size, VkBufferUsageFlags usage, MemoryType type) {
    y_debug_assert(buffer_size);

    const auto buffer = create_buffer(buffer_size, usage);
    auto memory = device_allocator().alloc(buffer, type);
    bind_buffer_memory(buffer, memory);

    return {buffer, std::move(memory)};
}



BufferBase::BufferBase(usize byte_size, BufferUsage usage, MemoryType type) : _size(byte_size), _usage(usage) {
    std::tie(_buffer, _memory) = alloc_buffer(byte_size, VkBufferUsageFlagBits(usage), type);
}

BufferBase::~BufferBase() {
    device_destroy( _buffer);
    device_destroy(std::move(_memory));
}

BufferUsage BufferBase::usage() const {
    return _usage;
}

usize BufferBase::byte_size() const {
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

VkDescriptorBufferInfo BufferBase::descriptor_info() const {
    VkDescriptorBufferInfo info = {};
    {
        info.buffer = _buffer;
        info.offset = 0;
        info.range = byte_size();
    }
    return info;

}

}

