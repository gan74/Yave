/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#ifndef YAVE_GRAPHICS_MEMORY_DEVICEMEMORY_H
#define YAVE_GRAPHICS_MEMORY_DEVICEMEMORY_H

#include <yave/graphics/vk/vk.h>
#include <yave/device/DeviceLinked.h>

namespace yave {

class DeviceMemoryHeapBase;

class DeviceMemory : NonCopyable, public DeviceLinked {

	public:
		DeviceMemory() = default;

		DeviceMemory(DeviceMemoryHeapBase* heap, vk::DeviceMemory memory, usize offset, usize size);
		DeviceMemory(DevicePtr dptr, vk::DeviceMemory memory, usize offset, usize size);

		~DeviceMemory();

		DeviceMemory(DeviceMemory&& other);
		DeviceMemory& operator=(DeviceMemory&& other);

		vk::DeviceMemory vk_memory() const;
		usize vk_offset() const;
		usize vk_size() const;

		DeviceMemoryHeapBase* heap() const;

	protected:
		void swap(DeviceMemory& other);

	private:
		friend class LifetimeManager;

		void free();

		NotOwner<DeviceMemoryHeapBase*> _heap = nullptr;
		vk::DeviceMemory _memory;
		usize _offset = 0;
		usize _size = 0;
};

}

#endif // YAVE_GRAPHICS_MEMORY_DEVICEMEMORY_H
