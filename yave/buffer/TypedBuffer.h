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
#ifndef YAVE_BUFFER_TYPEDBUFFER_H
#define YAVE_BUFFER_TYPEDBUFFER_H


#include <yave/yave.h>
#include "TypedMapping.h"

namespace yave {

template<typename Elem, BufferUsage Usage, MemoryFlags Flags = prefered_memory_flags(Usage), BufferTransfer Transfer = prefered_transfer(Flags)>
class TypedBuffer : public Buffer<Usage, Flags, Transfer> {

	using Base = Buffer<Usage, Flags, Transfer>;

	public:
		using value_type = Elem;

		TypedBuffer() {
		}

		TypedBuffer(DevicePtr dptr, const core::ArrayProxy<Elem>& data) : TypedBuffer(dptr, data.size()) {
			auto mapping = map();
			std::copy(data.begin(), data.end(), mapping.begin());
		}

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

		auto map() {
			return TypedMapping<value_type, Flags>(*this);
		}
};

}

#endif // YAVE_BUFFER_TYPEDBUFFER_H
