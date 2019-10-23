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

#include "DeviceMemoryAllocator.h"
#include "alloc.h"

namespace yave {

Y_TODO(DeviceAllocator should track allocation count)


usize DeviceMemoryAllocator::heap_size_for_type(MemoryType type) {
	if(type == MemoryType::Staging) {
		return default_heap_size / 8;
	}
	return default_heap_size;
}

usize DeviceMemoryAllocator::dedicated_threshold_for_type(MemoryType type) {
	return heap_size_for_type(type) / 2;
}


DeviceMemoryAllocator::DeviceMemoryAllocator(DevicePtr dptr) :
		DeviceLinked(dptr),
		_max_allocs(dptr->vk_limits().maxMemoryAllocationCount) {

	log_msg(fmt("Max device memory allocation count: %", _max_allocs));
}

DeviceMemory DeviceMemoryAllocator::dedicated_alloc(vk::MemoryRequirements reqs, MemoryType type) {
	y_profile();
	auto& heap = _dedicated_heaps[type];
	if(!heap) {
		heap = std::make_unique<DedicatedDeviceMemoryAllocator>(device(), type);
	}
	return std::move(heap->alloc(reqs).unwrap());
}

DeviceMemory DeviceMemoryAllocator::alloc(vk::MemoryRequirements reqs, MemoryType type) {
	y_profile();

	std::unique_lock lock(_lock);

	if(reqs.size >= dedicated_threshold_for_type(type)) {
		return dedicated_alloc(reqs, type);
	}

	if(reqs.alignment % DeviceMemoryHeap::alignment && DeviceMemoryHeap::alignment % reqs.alignment) {
		log_msg("Failed to align memory: defaulting to dedicated allocation.", Log::Warning);
		return dedicated_alloc(reqs, type);
	}

	auto& heaps =_heaps[HeapType{reqs.memoryTypeBits, type}];
	for(auto& heap : heaps) {
		if(auto r = heap->alloc(reqs)) {
			return std::move(r.unwrap());
		}
	}

	auto heap = std::make_unique<DeviceMemoryHeap>(device(), reqs.memoryTypeBits, type, heap_size_for_type(type));
	auto alloc = std::move(heap->alloc(reqs).unwrap());

	heaps.push_back(std::move(heap));

    return /*std::move*/(alloc);
}

DeviceMemory DeviceMemoryAllocator::alloc(vk::Image image) {
	return alloc(device()->vk_device().getImageMemoryRequirements(image), MemoryType::DeviceLocal);
}

DeviceMemory DeviceMemoryAllocator::alloc(vk::Buffer buffer, MemoryType type) {
	return alloc(device()->vk_device().getBufferMemoryRequirements(buffer), type);
}

}
