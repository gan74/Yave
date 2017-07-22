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
#ifndef YAVE_BUFFERS_BUFFER_H
#define YAVE_BUFFERS_BUFFER_H

#include "BufferUsage.h"
#include "BufferBase.h"

namespace yave {

template<BufferUsage Usage, MemoryType Memory = prefered_memory_flags(Usage), BufferTransfer Transfer = prefered_transfer(Memory)>
class Buffer : public BufferBase {

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

		Buffer() = default;

		// This is important: it prevent the ctor from being instanciated for Buffer specialisations that should not be created this way,
		// thus preventing static_assert from going off.
		template<typename = void>
		Buffer(DevicePtr dptr, usize byte_size) : BufferBase(dptr, byte_size, Usage, Memory, Transfer) {
			static_assert(Usage != BufferUsage::None || Transfer != BufferTransfer::None, "Buffers should not have Usage == BufferUsage::None");
			static_assert(Memory != MemoryType::DontCare, "Buffers should not have Memory == MemoryType::DontCare");
		}

		template<BufferUsage U, MemoryType M, BufferTransfer T, typename = std::enable_if_t<is_compatible(U, M, T)>>
		Buffer(Buffer<U, M, T>&& other) {
			swap(other);
		}

		template<BufferUsage U, MemoryType M, BufferTransfer T, typename = std::enable_if_t<is_compatible(U, M, T)>>
		Buffer& operator=(Buffer<U, M, T>&& other) {
			swap(other);
			return *this;
		}
};
}

#endif // YAVE_BUFFERS_BUFFER_H
