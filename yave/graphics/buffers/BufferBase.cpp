/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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

#include <yave/device/Device.h>

namespace yave {

static void bind_buffer_memory(DevicePtr dptr, vk::Buffer buffer, const DeviceMemory& memory) {
	dptr->vk_device().bindBufferMemory(buffer, memory.vk_memory(), memory.vk_offset());
}

static vk::Buffer create_buffer(DevicePtr dptr, usize byte_size, vk::BufferUsageFlags usage) {
	y_debug_assert(byte_size);
	if(usage & vk::BufferUsageFlagBits::eUniformBuffer) {
		if(byte_size > dptr->vk_limits().maxUniformBufferRange) {
			y_fatal("Uniform buffer size exceeds maxUniformBufferRange (%).", dptr->vk_limits().maxUniformBufferRange);
		}
	}
	return dptr->vk_device().createBuffer(vk::BufferCreateInfo()
			.setSize(byte_size)
			.setUsage(usage)
			.setSharingMode(vk::SharingMode::eExclusive)
		);
}

static std::tuple<vk::Buffer, DeviceMemory> alloc_buffer(DevicePtr dptr, usize buffer_size, vk::BufferUsageFlags usage, MemoryType type) {
	y_debug_assert(buffer_size);

	auto buffer = create_buffer(dptr, buffer_size, usage);
	auto memory = dptr->allocator().alloc(buffer, type);
	bind_buffer_memory(dptr, buffer, memory);

	return {buffer, std::move(memory)};
}



BufferBase::BufferBase(DevicePtr dptr, usize byte_size, BufferUsage usage, MemoryType type) : _size(byte_size), _usage(usage) {
	std::tie(_buffer, _memory) = alloc_buffer(dptr, byte_size, vk::BufferUsageFlagBits(usage), type);
}

BufferBase::~BufferBase() {
	if(device()) {
		device()->destroy(_buffer);
	}
}

DevicePtr BufferBase::device() const {
	return _memory.device();
}

BufferUsage BufferBase::usage() const {
	return _usage;
}

usize BufferBase::byte_size() const {
	return _size;
}

vk::Buffer BufferBase::vk_buffer() const {
	return _buffer;
}

const DeviceMemory& BufferBase::device_memory() const {
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
	std::swap(_size, other._size);
	std::swap(_buffer, other._buffer);
	std::swap(_usage, other._usage);
	std::swap(_memory, other._memory);
}

}
