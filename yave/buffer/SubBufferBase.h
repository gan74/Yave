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
#ifndef YAVE_BUFFER_SUBBUFFERBASE_H
#define YAVE_BUFFER_SUBBUFFERBASE_H

#include "GenericBuffer.h"

namespace yave {

class SubBufferBase : public DeviceLinked {

	public:
		usize byte_size() const {
			return _size;
		}

		usize byte_offset() const {
			return _offset;
		}

		vk::Buffer get_vk_buffer() const {
			return _buffer;
		}

		vk::DeviceMemory get_vk_device_memory() const {
			return _memory;
		}

		vk::DescriptorBufferInfo descriptor_info() const {
			return vk::DescriptorBufferInfo()
					.setBuffer(_buffer)
					.setOffset(_offset)
					.setRange(_size)
				;
		}

		SubBufferBase(const BufferBase &base) : SubBufferBase(base, 0, base.byte_size()) {
		}

		SubBufferBase() = default;

	protected:
		SubBufferBase(const BufferBase& base, usize off, usize len) :
				DeviceLinked(base.get_device()),
				_size(len),
				_offset(off),
				_buffer(base.get_vk_buffer()),
				_memory(base.get_vk_device_memory()) {
		}

	private:
		usize _size;
		usize _offset;
		vk::Buffer _buffer;
		vk::DeviceMemory _memory;
};

}

#endif // YAVE_BUFFER_SUBBUFFERBASE_H
