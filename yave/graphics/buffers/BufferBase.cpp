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

#include "BufferBase.h"

#include <yave/graphics/graphics.h>
#include <yave/graphics/device/deviceutils.h>
#include <yave/graphics/device/DeviceProperties.h>

#include <y/utils/format.h>

namespace yave {

u64 buffer_alignment_for_usage(BufferUsage usage) {
    u64 align = 1;
    const auto& props = device_properties();
    if((usage & BufferUsage::UniformBit) == BufferUsage::UniformBit) {
        align = std::max(props.uniform_buffer_alignment, align);
    }
    if((usage & BufferUsage::StorageBit) == BufferUsage::StorageBit) {
        align = std::max(props.storage_buffer_alignment, align);
    }
    if((usage & BufferUsage::AccelStructureScratchBit) == BufferUsage::AccelStructureScratchBit) {
        align = std::max(props.acceleration_structure_buffer_alignment, align);
    }
    return u64(align);
}


static std::tuple<VkHandle<VkBuffer>, DeviceMemory> alloc_buffer(u64 buffer_size, BufferUsage usage, MemoryType type, MemoryAllocFlags flags) {
    y_profile();

    unused(flags);

    y_debug_assert(buffer_size);

    y_always_assert(
        (usage & BufferUsage::UniformBit) != BufferUsage::UniformBit ||
        buffer_size <= device_properties().max_uniform_buffer_size,
        "Uniform buffer size exceeds maxUniformBufferRange ({})", device_properties().max_uniform_buffer_size
    );


    const u64 raytracing_bits = VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR;
    usage = raytracing_enabled() ? usage : (usage & BufferUsage(~raytracing_bits));

    y_debug_assert(usage != BufferUsage::None);

    VkBufferCreateInfo buffer_create_info = vk_struct();
    {
        buffer_create_info.size = buffer_size;
        buffer_create_info.usage = VkBufferUsageFlags(usage) | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
        buffer_create_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }

    VmaAllocationCreateInfo alloc_create_info = {};
    switch(type) {
        case MemoryType::CpuVisible:
            alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_HOST;
            alloc_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT  | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        break;

        case MemoryType::Staging:
            alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
            alloc_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT  | VMA_ALLOCATION_CREATE_MAPPED_BIT;
        break;

        default:
            alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
        break;
    }

    const u64 alignment = buffer_alignment_for_usage(usage);

    VmaAllocation alloc = {};
    VkHandle<VkBuffer> buffer;
    vk_check(vmaCreateBufferWithAlignment(device_allocator(), &buffer_create_info, &alloc_create_info, alignment, buffer.get_ptr_for_init(), &alloc, nullptr));

    return {std::move(buffer), alloc};
}






SubBufferBase::SubBufferBase(const BufferBase& base, u64 byte_len, u64 byte_off) :
        _size(byte_len),
        _offset(byte_off),
        _buffer(base.vk_buffer()),
        _address(base.vk_device_address() + byte_off),
        _memory(base.device_memory()) {

    y_debug_assert(base.byte_size() >= byte_len + byte_off);
}

SubBufferBase::SubBufferBase(const SubBufferBase& base, u64 byte_len, u64 byte_off) :
        _size(byte_len),
        _offset(byte_off + base.byte_offset()),
        _buffer(base.vk_buffer()),
        _address(base.vk_device_address() + byte_off),
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


u64 SubBufferBase::byte_size() const {
    return _size;
}

u64 SubBufferBase::byte_offset() const {
    return _offset;
}

VkBuffer SubBufferBase::vk_buffer() const {
    return _buffer;
}

VkDeviceAddress SubBufferBase::vk_device_address() const {
    return _address;
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

bool SubBufferBase::operator==(const SubBufferBase& other) const {
    return _address == other._address;
}

bool SubBufferBase::operator!=(const SubBufferBase& other) const {
    return !operator==(other);
}




BufferBase::BufferBase(u64 byte_size, BufferUsage usage, MemoryType type, MemoryAllocFlags alloc_flags) : _size(byte_size), _usage(usage) {
    std::tie(_buffer, _memory) = alloc_buffer(byte_size, usage, type, alloc_flags);

    {
        VkBufferDeviceAddressInfo info = vk_struct();
        info.buffer = _buffer;

        _address = vkGetBufferDeviceAddress(vk_device(), &info);
    }
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

VkDeviceAddress BufferBase::vk_device_address() const {
    return _address;
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

