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
#ifndef YAVE_GRAPHICS_MEMORY_ALLOC_H
#define YAVE_GRAPHICS_MEMORY_ALLOC_H

#include <yave/graphics/vk/vk.h>
#include <yave/device/Device.h>

#include "MemoryType.h"

// THIS FILE SHOULD NOT BE INCLUDED OUTSIDE OF MEMORY'S CPPs !!!

namespace yave {

static const vk::MemoryPropertyFlags dont_care_flags[] = {
	vk::MemoryPropertyFlags()
};

static const vk::MemoryPropertyFlags device_local_flags[] = {
	vk::MemoryPropertyFlagBits::eDeviceLocal,
	vk::MemoryPropertyFlags()
};

static const vk::MemoryPropertyFlags cpu_visible_flags[] = {
	vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCached,
	vk::MemoryPropertyFlagBits::eHostVisible,
	vk::MemoryPropertyFlags()
};

static const vk::MemoryPropertyFlags staging_flags[] = {
	vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCached | vk::MemoryPropertyFlagBits::eDeviceLocal,
	vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eDeviceLocal,
	vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCached,
	vk::MemoryPropertyFlagBits::eHostVisible,
	vk::MemoryPropertyFlags()
};



static const vk::MemoryPropertyFlags* memory_type_flags[] = {
	dont_care_flags,    // DontCare
	device_local_flags, // DeviceLocal
	cpu_visible_flags,  // CpuVisible
	staging_flags		// Staging
};


inline u32 get_memory_type(const vk::PhysicalDeviceMemoryProperties& properties, u32 type_filter, MemoryType type) {
	for(const vk::MemoryPropertyFlags* type_flags = memory_type_flags[uenum(type)]; *type_flags; ++type_flags) {
		const vk::MemoryPropertyFlags flags = *type_flags;
		for(u32 i = 0; i != properties.memoryTypeCount; ++i) {
			const auto memory_type = properties.memoryTypes[i];
			if(type_filter & (1 << i) && (memory_type.propertyFlags & flags) == flags) {
				return i;
			}
		}
	}

	return y_fatal("Unable to allocate device memory.");
}



inline vk::DeviceMemory alloc_memory(DevicePtr dptr, usize size, u32 type_bits, MemoryType type) {
	y_profile();
	try {
		return dptr->vk_device().allocateMemory(vk::MemoryAllocateInfo()
					.setAllocationSize(size)
					.setMemoryTypeIndex(get_memory_type(dptr->physical_device().vk_memory_properties(), type_bits, type))
				);
	} catch(std::exception& e) {
		y_fatal("Failed to allocate memory: %", e.what());
	}
}

inline vk::DeviceMemory alloc_memory(DevicePtr dptr, vk::MemoryRequirements reqs, MemoryType type) {
	return alloc_memory(dptr, reqs.size, reqs.memoryTypeBits, type);
}

}

#endif // YAVE_GRAPHICS_MEMORY_ALLOC_H
