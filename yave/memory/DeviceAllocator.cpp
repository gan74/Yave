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

#include "DeviceAllocator.h"
#include "alloc.h"

namespace yave {

#warning DeviceAllocator is not thread safe
#warning DeviceAllocator does not count allocs

DeviceAllocator::DeviceAllocator(DevicePtr dptr) :
		DeviceLinked(dptr),
		_max_allocs(dptr->vk_limits().maxMemoryAllocationCount) {

	log_msg("Max device memory allocation count: "_s + _max_allocs);
}

DeviceMemory DeviceAllocator::dedicated_alloc(vk::MemoryRequirements reqs, MemoryType type) {
	if(auto it = _dedicated_heaps.find(type); it != _dedicated_heaps.end()) {
		return std::move(it->second->alloc(reqs).unwrap());
	}
	return std::move((_dedicated_heaps[type] = new DedicatedDeviceMemoryHeap(device(), type))->alloc(reqs).unwrap());
}

DeviceMemory DeviceAllocator::alloc(vk::MemoryRequirements reqs, MemoryType type) {
	if(reqs.size >= dedicated_threshold) {
		return dedicated_alloc(reqs, type);
	}
	if(reqs.alignment % DeviceMemoryHeap::alignment && DeviceMemoryHeap::alignment % reqs.alignment) {
		log_msg("Failed to align memory: defaulting to dedicated allocation.", Log::Warning);
		return dedicated_alloc(reqs, type);
	}

	auto& heaps =_heaps[HeapType{reqs.memoryTypeBits, type}];
	for(auto& heap : heaps) {
		if(auto r = heap->alloc(reqs); r.is_ok()) {
			return std::move(r.unwrap());
		}
	}

	auto heap = core::Unique(new DeviceMemoryHeap(device(), reqs.memoryTypeBits, type));
	auto alloc = std::move(heap->alloc(reqs).unwrap());

	heaps.push_back(std::move(heap));

	return std::move(alloc);
}


DeviceMemory DeviceAllocator::alloc(vk::Image image) {
	return alloc(device()->vk_device().getImageMemoryRequirements(image), MemoryType::DeviceLocal);
}

DeviceMemory DeviceAllocator::alloc(vk::Buffer buffer, MemoryType type) {
	return alloc(device()->vk_device().getBufferMemoryRequirements(buffer), type);
}


void DeviceAllocator::dump_info() const {
	log_msg("Allocator:");
	log_msg("  maximum allocations = "_s + _max_allocs);
	/*log_msg("  dedicated allocations = "_s + _dedicated);
	log_msg("  heap count = "_s + _heap_count);
	log_msg("  total allocations = "_s + (_dedicated + _heap_count));*/
	for(auto& heaps : _heaps) {
		log_msg("  Memory type = "_s + usize(heaps.first.second) + "|" + heaps.first.first);
		for(auto& h : heaps.second) {
			log_msg("    heap:");
			log_msg("      total: "_s + (h->heap_size / 1024) + " KB");
			log_msg("      free : "_s + (h->available() / 1024) + " KB");
		}
	}
}

}
