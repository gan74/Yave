/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
#ifndef YAVE_MEMORY_ALLOC_H
#define YAVE_MEMORY_ALLOC_H

#include <yave/vk/vk.h>
#include <yave/device/Device.h>

#include "MemoryType.h"

// THIS FILE SHOULD NOT BE INCLUDED OUTSIDE OF MEMORY'S CPPs !!!

namespace yave {

inline u32 get_memory_type(const vk::PhysicalDeviceMemoryProperties& properties, u32 type_filter, MemoryType type) {

	// try with optional flags
	vk::MemoryPropertyFlagBits optional_flags = optional_memory_flags(type);
	if(optional_flags != vk::MemoryPropertyFlagBits()) {
		vk::MemoryPropertyFlags flags = vk::MemoryPropertyFlagBits(type) | optional_flags;
		for(u32 i = 0; i != properties.memoryTypeCount; ++i) {
			auto memory_type = properties.memoryTypes[i];
			if(type_filter & (1 << i) && (memory_type.propertyFlags & flags) == flags) {
				return i;
			}
		}
	}

	// try without
	vk::MemoryPropertyFlags flags = vk::MemoryPropertyFlagBits(type);
	for(u32 i = 0; i != properties.memoryTypeCount; ++i) {
		auto memory_type = properties.memoryTypes[i];
		if(type_filter & (1 << i) && (memory_type.propertyFlags & flags) == flags) {
			return i;
		}
	}

	return y_fatal("Unable to allocate device memory.");
}



inline vk::DeviceMemory alloc_memory(DevicePtr dptr, usize size, u32 type_bits, MemoryType type) {
	return dptr->vk_device().allocateMemory(vk::MemoryAllocateInfo()
				.setAllocationSize(size)
				.setMemoryTypeIndex(get_memory_type(dptr->physical_device().vk_memory_properties(), type_bits, type))
			);
}

inline vk::DeviceMemory alloc_memory(DevicePtr dptr, vk::MemoryRequirements reqs, MemoryType type) {
	return alloc_memory(dptr, reqs.size, reqs.memoryTypeBits, type);
}

}

#endif // YAVE_MEMORY_ALLOC_H
