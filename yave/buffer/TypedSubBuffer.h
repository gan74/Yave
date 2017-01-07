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
#ifndef YAVE_BUFFER_TYPEDSUBBUFFER_H
#define YAVE_BUFFER_TYPEDSUBBUFFER_H

#include "TypedBuffer.h"
#include "SubBuffer.h"

namespace yave {

template<typename Elem, BufferUsage Usage, MemoryFlags Flags = prefered_memory_flags<Usage>()>
class TypedSubBuffer : public SubBuffer<Usage, Flags> {

	using Base = SubBuffer<Usage, Flags>;

	public:
		using Element = Elem;

		template<BufferUsage BuffUsage>
		TypedSubBuffer(const TypedBuffer<Elem, BuffUsage, Flags>& buffer, usize offset, usize count) : Base(buffer, offset * sizeof(Elem), count * sizeof(Elem)) {
		}

		template<BufferUsage BuffUsage>
		explicit TypedSubBuffer(const TypedBuffer<Elem, BuffUsage, Flags>& buffer, usize offset = 0) : Base(buffer, offset * sizeof(Elem)) {
		}

		template<BufferUsage BuffUsage>
		TypedSubBuffer(const Buffer<BuffUsage, Flags>& buffer, usize byte_offset, usize count) : Base(buffer, byte_offset, count * sizeof(Elem)) {
		}

		template<BufferUsage BuffUsage>
		explicit TypedSubBuffer(const Buffer<BuffUsage, Flags>& buffer, usize byte_offset = 0) : Base(buffer, byte_offset, buffer.byte_size()) {
		}


		usize size() const {
			return this->byte_size() / sizeof(Elem);
		}

		usize offset() const {
			return this->byte_offset() / sizeof(Elem);
		}

		TypedMapping<Element, Flags> map() {
			return TypedMapping<Element, Flags>(*this);
		}
};

}

#endif // YAVE_BUFFER_TYPEDSUBBUFFER_H
