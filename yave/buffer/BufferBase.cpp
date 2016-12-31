/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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

#include <yave/Device.h>

namespace yave {

static u32 get_memory_type(const vk::PhysicalDeviceMemoryProperties& properties, u32 type_filter, MemoryFlags flags) {
	auto mem_flags = vk::MemoryPropertyFlagBits(flags);
	for(u32 i = 0; i != properties.memoryTypeCount; i++) {
		auto memory_type = properties.memoryTypes[i];
		if(type_filter & (1 << i) && (memory_type.propertyFlags & mem_flags) == mem_flags) {
			return i;
		}
	}
	return fatal("Unable to alloc device memory");
}

static vk::MemoryRequirements get_memory_reqs(DevicePtr dptr, vk::Buffer buffer) {
	return dptr->vk_device().getBufferMemoryRequirements(buffer);
}


static void bind_buffer_memory(DevicePtr dptr, vk::Buffer buffer, vk::DeviceMemory memory) {
	dptr->vk_device().bindBufferMemory(buffer, memory, 0);
}

static vk::DeviceMemory alloc_memory(DevicePtr dptr, vk::MemoryRequirements reqs, MemoryFlags flags) {
	return dptr->vk_device().allocateMemory(vk::MemoryAllocateInfo()
			.setAllocationSize(reqs.size)
			.setMemoryTypeIndex(get_memory_type(dptr->physical_device().vk_memory_properties(), reqs.memoryTypeBits, flags))
		);
}

static vk::Buffer create_buffer(DevicePtr dptr, usize byte_size, vk::BufferUsageFlags usage) {
	return dptr->vk_device().createBuffer(vk::BufferCreateInfo()
			.setSize(byte_size)
			.setUsage(usage)
			.setSharingMode(vk::SharingMode::eExclusive)
		);
}

static std::tuple<vk::Buffer, vk::DeviceMemory> alloc_buffer(DevicePtr dptr, usize buffer_size, vk::BufferUsageFlags usage, MemoryFlags flags) {
	if(!buffer_size) {
		fatal("Can not allocate 0 sized buffer.");
	}
	auto buffer = create_buffer(dptr, buffer_size, usage);
	auto memory = alloc_memory(dptr, get_memory_reqs(dptr, buffer), flags);
	bind_buffer_memory(dptr, buffer, memory);

	return std::tuple<vk::Buffer, vk::DeviceMemory>(buffer, memory);
}

template<typename T>
vk::BufferUsageFlags to_vk_flags(T t) {
	auto value = vk::BufferUsageFlagBits(t);
	return value | value;
}





usize BufferBase::byte_size() const {
	return _size;
}

vk::Buffer BufferBase::vk_buffer() const {
	return _buffer;
}

vk::DeviceMemory BufferBase::vk_device_memory() const {
	return _memory;
}

vk::DescriptorBufferInfo BufferBase::descriptor_info() const {
	return vk::DescriptorBufferInfo()
			.setBuffer(_buffer)
			.setOffset(0)
			.setRange(byte_size())
		;
}

void BufferBase::swap(BufferBase& other) {
	DeviceLinked::swap(other);
	std::swap(_size, other._size);
	std::swap(_buffer, other._buffer);
	std::swap(_memory, other._memory);
}

BufferBase::BufferBase(DevicePtr dptr, usize byte_size, BufferUsage usage, MemoryFlags flags, BufferTransfer transfer) : DeviceLinked(dptr), _size(byte_size) {
	auto tpl = alloc_buffer(dptr, byte_size, to_vk_flags(usage) | to_vk_flags(transfer), flags);
	_buffer = std::get<0>(tpl);
	_memory = std::get<1>(tpl);
}

BufferBase::~BufferBase() {
	destroy(_memory);
	destroy(_buffer);
}

}
