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
		SubBuffer() = default;

		template<BufferUsage U, MemoryType M, BufferTransfer T, typename = std::enable_if_t<is_compatible(U, M, T)>>
		SubBuffer(const SubBuffer<U, M, T>& buffer) : SubBufferBase(buffer) {
		}

		template<BufferUsage U, MemoryType M, BufferTransfer T, typename = std::enable_if_t<is_compatible(U, M, T)>>
		SubBuffer(const Buffer<U, M, T>& buffer, usize byte_off = 0) : SubBufferBase(buffer, byte_off, buffer.byte_size() - byte_off) {
		}

		template<BufferUsage U, MemoryType M, BufferTransfer T, typename = std::enable_if_t<is_compatible(U, M, T)>>
		SubBuffer(const Buffer<U, M, T>& buffer, usize byte_off, usize byte_len) : SubBufferBase(buffer, byte_off, byte_len) {
		}
};


template<BufferUsage U, MemoryType M, BufferTransfer T>
SubBuffer(const Buffer<U, M, T>&) -> SubBuffer<U, M, T>;


}

#endif // YAVE_BUFFERS_SUBBUFFER_H
