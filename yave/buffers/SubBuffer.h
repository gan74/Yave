/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#ifndef YAVE_BUFFERS_SUBBUFFER_H
#define YAVE_BUFFERS_SUBBUFFER_H

#include "SubBufferBase.h"
#include "Buffer.h"

namespace yave {

template<BufferUsage Usage = BufferUsage::None, MemoryType Memory = MemoryType::DontCare, BufferTransfer Transfer = BufferTransfer::None>
class SubBuffer : public SubBufferBase {

	protected:
		template<typename T>
		static constexpr bool is_compatible(T a, T b) {
			return (uenum(a) & uenum(b)) == uenum(b);
		}

		static constexpr bool is_compatible(BufferUsage U, MemoryType M, BufferTransfer T) {
			return is_compatible(U, Usage) && is_compatible(M, Memory) && is_compatible(T, Transfer);
		}

	public:
		static constexpr BufferUsage usage = Usage;
		static constexpr MemoryType memory_type = Memory;
		static constexpr BufferTransfer buffer_transfer = Transfer;

		using sub_buffer_type = SubBuffer<Usage, memory_type, buffer_transfer>;

		SubBuffer() = default;

		template<BufferUsage U, MemoryType M, BufferTransfer T>
		SubBuffer(const SubBuffer<U, M, T>& buffer) : SubBufferBase(buffer) {
			static_assert(is_compatible(U, M, T));
		}

		template<BufferUsage U, MemoryType M, BufferTransfer T>
		SubBuffer(const Buffer<U, M, T>& buffer) : SubBufferBase(buffer, 0, buffer.byte_size()) {
			static_assert(is_compatible(U, M, T));
		}

		// these are dangerous with typedwrapper as it's never clear what is in byte and what isn't.
		// todo find some way to make this better

		template<BufferUsage U, MemoryType M, BufferTransfer T>
		SubBuffer(const Buffer<U, M, T>& buffer, usize byte_off) : SubBufferBase(buffer, byte_off, buffer.byte_size() - byte_off) {
			static_assert(is_compatible(U, M, T));
		}

		template<BufferUsage U, MemoryType M, BufferTransfer T>
		SubBuffer(const Buffer<U, M, T>& buffer, usize byte_off, usize byte_len) : SubBufferBase(buffer, byte_off, byte_len) {
			static_assert(is_compatible(U, M, T));
		}
};


template<BufferUsage U, MemoryType M, BufferTransfer T>
SubBuffer(const Buffer<U, M, T>&) -> SubBuffer<U, M, T>;


}

#endif // YAVE_BUFFERS_SUBBUFFER_H
