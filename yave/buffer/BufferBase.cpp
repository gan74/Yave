/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#include "BufferBase.h"

#include <yave/Device.h>

namespace yave {

static usize get_memory_type(const vk::PhysicalDeviceMemoryProperties& properties, u32 type_filter, MemoryFlags flags) {
	auto mem_flags = vk::MemoryPropertyFlagBits(flags);
	for(usize i = 0; i != properties.memoryTypeCount; i++) {
		auto memory_type = properties.memoryTypes[i];
		if(type_filter & (1 << i) && (memory_type.propertyFlags & mem_flags) == mem_flags) {
			return i;
		}
	}
	return fatal("Unable to alloc device memory");
}

static vk::MemoryRequirements get_memory_reqs(DevicePtr dptr, vk::Buffer buffer) {
	return dptr->get_vk_device().getBufferMemoryRequirements(buffer);
}


static void bind_buffer_memory(DevicePtr dptr, vk::Buffer buffer, vk::DeviceMemory memory) {
	dptr->get_vk_device().bindBufferMemory(buffer, memory, 0);
}

static vk::DeviceMemory alloc_memory(DevicePtr dptr, vk::MemoryRequirements reqs, MemoryFlags flags) {
	return dptr->get_vk_device().allocateMemory(vk::MemoryAllocateInfo()
			.setAllocationSize(reqs.size)
			.setMemoryTypeIndex(get_memory_type(dptr->get_physical_device().get_vk_memory_properties(), reqs.memoryTypeBits, flags))
		);
}

static vk::Buffer create_buffer(DevicePtr dptr, usize byte_size, vk::BufferUsageFlags usage) {
	return dptr->get_vk_device().createBuffer(vk::BufferCreateInfo()
			.setSize(byte_size)
			.setUsage(usage)
			.setSharingMode(vk::SharingMode::eExclusive)
		);
}

static std::tuple<vk::Buffer, vk::DeviceMemory> alloc_buffer(DevicePtr dptr, usize buffer_size, vk::BufferUsageFlags usage, MemoryFlags flags) {
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

vk::Buffer BufferBase::get_vk_buffer() const {
	return _buffer;
}

vk::DeviceMemory BufferBase::get_vk_device_memory() const {
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
#ifdef YAVE_DEBUG_BUFFERS
	if(_mapped) {
		fatal("Buffer not unmaped before being moved");
	}
#endif
}

BufferBase::BufferBase(DevicePtr dptr, usize byte_size, BufferUsage usage, MemoryFlags flags, BufferTransfer transfer) : DeviceLinked(dptr), _size(byte_size) {
	auto tpl = alloc_buffer(dptr, byte_size, to_vk_flags(usage) | to_vk_flags(transfer), flags);
	_buffer = std::get<0>(tpl);
	_memory = std::get<1>(tpl);

#ifdef YAVE_DEBUG_BUFFERS
	_mapped = 0;
#endif
}

BufferBase::~BufferBase() {
	destroy(_memory);
	destroy(_buffer);
#ifdef YAVE_DEBUG_BUFFERS
	if(_mapped) {
		fatal("Buffer not unmaped before being destroyed");
	}
#endif
}

}
