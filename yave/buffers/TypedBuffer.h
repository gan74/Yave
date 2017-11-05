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
#ifndef YAVE_BUFFERS_TYPEDBUFFER_H
#define YAVE_BUFFERS_TYPEDBUFFER_H

#include <yave/yave.h>

#include "Buffer.h"

namespace yave {

template<typename Elem, BufferUsage Usage, MemoryType Memory = prefered_memory_type(Usage), BufferTransfer Transfer = prefered_transfer(Memory)>
class TypedBuffer : public Buffer<Usage, Memory, Transfer> {

	using Base = Buffer<Usage, Memory, Transfer>;

	public:
		using value_type = Elem;

		TypedBuffer() = default;

		TypedBuffer(DevicePtr dptr, const core::ArrayView<Elem>& data); // See TypedMapping.h

		TypedBuffer(DevicePtr dptr, usize elem_count) : Base(dptr, elem_count * sizeof(Elem)) {
		}

		TypedBuffer(TypedBuffer&& other) {
			this->swap(other);
		}

		TypedBuffer& operator=(TypedBuffer&& other) {
			this->swap(other);
			return *this;
		}

		usize size() const {
			return this->byte_size() / sizeof(Elem);
		}
};

}

#endif // YAVE_BUFFERS_TYPEDBUFFER_H
