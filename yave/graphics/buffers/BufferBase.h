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
#ifndef YAVE_GRAPHICS_BUFFERS_BUFFERBASE_H
#define YAVE_GRAPHICS_BUFFERS_BUFFERBASE_H

#include "BufferUsage.h"
#include <yave/device/DeviceLinked.h>
#include <yave/graphics/memory/DeviceMemory.h>

namespace yave {

class BufferBase : NonCopyable {

	public:
		~BufferBase();

		DevicePtr device() const;
		bool is_null() const;

		BufferUsage usage() const;
		usize byte_size() const;
		const DeviceMemory& device_memory() const;

		vk::DescriptorBufferInfo descriptor_info() const;

		vk::Buffer vk_buffer() const;

	protected:
		BufferBase() = default;
		BufferBase(BufferBase&&) = default;
		BufferBase& operator=(BufferBase&&) = default;

		BufferBase(DevicePtr dptr, usize byte_size, BufferUsage usage, MemoryType type);

	private:
		usize _size = 0;
		SwapMove<vk::Buffer> _buffer;
		BufferUsage _usage = BufferUsage::None;
		DeviceMemory _memory;
};

static_assert(is_safe_base<BufferBase>::value);

}


#endif // YAVE_GRAPHICS_BUFFERS_BUFFERBASE_H
