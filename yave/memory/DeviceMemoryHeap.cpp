/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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

#include "DeviceMemoryHeap.h"
#include "alloc.h"

#include <numeric>
#include <mutex>

namespace yave {

usize DeviceMemoryHeap::FreeBlock::end_offset() const {
	return offset + size;
}

bool DeviceMemoryHeap::FreeBlock::contiguous(const FreeBlock& blck) const {
	return end_offset() == blck.offset || blck.end_offset() == offset;
}

void DeviceMemoryHeap::FreeBlock::merge(const FreeBlock& block) {
	if(!contiguous(block)) {
		fatal("Memory blocks are not contiguous.");
	}
	std::tie(offset, size) = std::tuple{std::min(offset, block.offset), size + block.size};
}


DeviceMemoryHeap::DeviceMemoryHeap(DevicePtr dptr, u32 type_bits, MemoryType type) :
		DeviceMemoryHeapBase(dptr),
		_memory(alloc_memory(dptr, heap_size, type_bits, type)),
		_blocks({FreeBlock{0, heap_size}}) {
}

DeviceMemoryHeap::~DeviceMemoryHeap() {
	device()->vk_device().freeMemory(_memory);
}

DeviceMemory DeviceMemoryHeap::create(usize offset, usize size) {
	return DeviceMemory(this, _memory, offset, size);
}

core::Result<DeviceMemory> DeviceMemoryHeap::alloc(vk::MemoryRequirements reqs) {
	usize size = reqs.size;
	for(auto it = _blocks.begin(); it != _blocks.end(); ++it) {
		usize offset = it->offset;

		if(offset % reqs.alignment) {
			fatal("Failed to allign");
		}

		if(it->size == size) {
			_blocks.erase_unordered(it);
			return core::Ok(create(offset, size));
		} else if(it->size > size) {
			it->offset += size;
			it->size -= size;
			return core::Ok(create(offset, size));
		}
	}
	return core::Err();
}

void DeviceMemoryHeap::free(const DeviceMemory& memory) {
	FreeBlock block{memory.vk_offset(), memory.vk_size()};
	for(auto& b : _blocks) {
		if(b.contiguous(block)) {
			b.merge(block);
			return;
		}
	}
	_blocks << block;
}

void* DeviceMemoryHeap::map(const DeviceMemoryView& view) {
	std::unique_lock lock(_lock);
	if(!_mapped) {
		_mapping = static_cast<u8*>(device()->vk_device().mapMemory(_memory, 0, heap_size));
	}
	++_mapped;
	return static_cast<u8*>(_mapping + view.vk_offset());
}

void DeviceMemoryHeap::unmap(const DeviceMemoryView&) {
	std::unique_lock lock(_lock);
	if(!--_mapped) {
		device()->vk_device().unmapMemory(_memory);
		_mapping = nullptr;
	}
}

usize DeviceMemoryHeap::available() const {
	usize tot = 0;
	for(const auto& b : _blocks) {
		tot += b.size;
	}
	return tot;
}

bool DeviceMemoryHeap::mapped() const {
	return _mapped;
}

}
