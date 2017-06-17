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
#ifndef YAVE_BUFFER_SUBBUFFERBASE_H
#define YAVE_BUFFER_SUBBUFFERBASE_H

#include "Buffer.h"

namespace yave {

class SubBufferBase : public DeviceLinked {

	public:
		SubBufferBase() = default;

		SubBufferBase(const BufferBase& base, usize off, usize len) :
				DeviceLinked(base.device()),
				_size(len),
				_offset(off),
				_buffer(base.vk_buffer()),
				_memory(base.vk_device_memory()) {
		}

		explicit SubBufferBase(const BufferBase& base) : SubBufferBase(base, 0, base.byte_size()) {
		}

		usize byte_size() const {
			return _size;
		}

		usize byte_offset() const {
			return _offset;
		}

		vk::Buffer vk_buffer() const {
			return _buffer;
		}

		vk::DeviceMemory vk_device_memory() const {
			return _memory;
		}

		vk::DescriptorBufferInfo descriptor_info() const {
			return vk::DescriptorBufferInfo()
					.setBuffer(_buffer)
					.setOffset(_offset)
					.setRange(_size)
				;
		}

	private:
		usize _size = 0;
		usize _offset = 0;
		NotOwner<vk::Buffer> _buffer;
		NotOwner<vk::DeviceMemory> _memory;
};

// in this case it's ok
//static_assert(is_safe_base<SubBufferBase>::value);

}

#endif // YAVE_BUFFER_SUBBUFFERBASE_H
