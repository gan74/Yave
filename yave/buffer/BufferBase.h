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
#ifndef YAVE_BUFFER_BUFFERBASE_H
#define YAVE_BUFFER_BUFFERBASE_H

#include "BufferUsage.h"
#include <yave/DeviceLinked.h>

namespace yave {

class BufferBase : NonCopyable, public DeviceLinked {

	public:
		usize byte_size() const;
		vk::Buffer vk_buffer() const;
		vk::DeviceMemory vk_device_memory() const;

		vk::DescriptorBufferInfo descriptor_info() const;

		~BufferBase();

	protected:
		void swap(BufferBase& other);

		BufferBase() = default;
		BufferBase(DevicePtr dptr, usize byte_size, BufferUsage usage, MemoryFlags flags, BufferTransfer transfer);

	private:
		usize _size = 0;
		vk::Buffer _buffer;
		vk::DeviceMemory _memory;
};

}


#endif // YAVE_BUFFER_BUFFERBASE_H
