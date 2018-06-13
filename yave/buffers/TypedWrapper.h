/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
#ifndef YAVE_BUFFERS_TYPEDWRAPPER_H
#define YAVE_BUFFERS_TYPEDWRAPPER_H

#include "Mapping.h"
#include "Buffer.h"
#include "SubBuffer.h"

namespace yave {
namespace detail {

template<typename T>
struct is_buffer {
	static constexpr bool value = false;
};

template<auto... Args>
struct is_buffer<Buffer<Args...>> {
	static constexpr bool value = true;
};

template<typename T>
struct is_sub_buffer {
	static constexpr bool value = false;
};

template<auto... Args>
struct is_sub_buffer<SubBuffer<Args...>> {
	static constexpr bool value = true;
};

}

template<typename Elem, typename Buff>
class TypedWrapper : public Buff {

	static constexpr bool is_sub = detail::is_sub_buffer<Buff>::value;
	static constexpr bool is_buf = detail::is_buffer<Buff>::value;
	static_assert(is_buf || is_sub);

	public:
		using Buff::Buff;

		using sub_buffer_type = TypedWrapper<Elem, typename Buff::sub_buffer_type>;

		using value_type = Elem;

		TypedWrapper() = default;

		TypedWrapper(DevicePtr dptr, usize elem_count) : Buff(dptr, elem_count * sizeof(Elem)) {
		}

		usize size() const {
			return this->byte_size() / sizeof(Elem);
		}

		usize offset() const {
			if constexpr(is_sub) {
				return this->byte_offset() / sizeof(Elem);
			} else {
				return 0;
			}
		}
};

template<typename Elem, BufferUsage Usage, MemoryType Memory = prefered_memory_type(Usage), BufferTransfer Transfer = prefered_transfer(Memory)>
using TypedBuffer = TypedWrapper<Elem, Buffer<Usage, Memory, Transfer>>;

template<typename Elem, BufferUsage Usage = BufferUsage::None, MemoryType Memory = MemoryType::DontCare, BufferTransfer Transfer = BufferTransfer::None>
using TypedSubBuffer = TypedWrapper<Elem, SubBuffer<Usage, Memory, Transfer>>;


template<typename Elem>
class TypedMapping : public Mapping {

	public:
		//using Mapping::Mapping;

		using iterator = Elem* ;
		using const_iterator = Elem const* ;
		using value_type = Elem;

		template<BufferUsage Usage, BufferTransfer Transfer>
		explicit TypedMapping(TypedBuffer<Elem, Usage, MemoryType::CpuVisible, Transfer>& buffer) : Mapping(buffer) {
		}

		template<BufferUsage Usage, BufferTransfer Transfer>
		explicit TypedMapping(TypedSubBuffer<Elem, Usage, MemoryType::CpuVisible, Transfer>& buffer) : Mapping(buffer) {
		}

		usize size() const {
			return this->byte_size() / sizeof(Elem);
		}

		iterator begin() {
			return reinterpret_cast<iterator>(this->data());
		}

		iterator end() {
			return begin() + size();
		}

		const_iterator begin() const {
			return reinterpret_cast<const_iterator>(this->data());
		}

		const_iterator end() const {
			return begin() + size();
		}

		const_iterator cbegin() const {
			return begin();
		}

		const_iterator cend() const {
			return end();
		}

		value_type& operator[](usize i) {
			return begin()[i];
		}

		const value_type& operator[](usize i) const{
			return begin()[i];
		}

};

template<typename Elem, BufferUsage Usage, BufferTransfer Transfer>
TypedMapping(TypedBuffer<Elem, Usage, MemoryType::CpuVisible, Transfer>&) -> TypedMapping<Elem>;

template<typename Elem, BufferUsage Usage, BufferTransfer Transfer>
TypedMapping(TypedSubBuffer<Elem, Usage, MemoryType::CpuVisible, Transfer>&) -> TypedMapping<Elem>;

}

#endif // YAVE_BUFFERS_TYPEDWRAPPER_H
