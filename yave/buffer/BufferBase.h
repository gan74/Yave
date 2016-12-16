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
#ifndef YAVE_BUFFER_BUFFERBASE_H
#define YAVE_BUFFER_BUFFERBASE_H

#include "buffers.h"
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

		BufferBase();
		BufferBase(DevicePtr dptr, usize byte_size, BufferUsage usage, MemoryFlags flags, BufferTransfer transfer);

	private:
		usize _size;
		vk::Buffer _buffer;
		vk::DeviceMemory _memory;
};

}


#endif // YAVE_BUFFER_BUFFERBASE_H
