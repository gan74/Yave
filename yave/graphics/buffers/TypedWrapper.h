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
#ifndef YAVE_GRAPHICS_BUFFERS_TYPEDWRAPPER_H
#define YAVE_GRAPHICS_BUFFERS_TYPEDWRAPPER_H

#include "Mapping.h"
#include "Buffer.h"
#include "SubBuffer.h"

namespace yave {

template<typename Elem, typename Buff>
class TypedWrapper final : public Buff {

	static constexpr bool is_sub = std::is_base_of_v<SubBufferBase, Buff>;
	static constexpr bool is_buf = std::is_base_of_v<BufferBase, Buff>;
	static_assert(is_buf || is_sub);

	public:
		using Buff::usage;
		using Buff::memory_type;
		using Buff::buffer_transfer;

		using sub_buffer_type = TypedWrapper<Elem, typename Buff::sub_buffer_type>;
		using base_buffer_type = Buffer<usage, memory_type, buffer_transfer>;

		using value_type = Elem;


		static usize total_byte_size(usize size) {
			return size * sizeof(value_type);
		}


		TypedWrapper() = default;

		TypedWrapper(DevicePtr dptr, usize elem_count) : Buff(dptr, elem_count * sizeof(value_type)) {
		}

		template<BufferUsage U, MemoryType M, BufferTransfer T>
		TypedWrapper(const Buffer<U, M, T>& buffer) : Buff(buffer) {
		}

		template<BufferUsage U, MemoryType M, BufferTransfer T>
		TypedWrapper(const SubBuffer<U, M, T>& buffer) : Buff(buffer) {
		}

		template<BufferUsage U, MemoryType M, BufferTransfer T>
		TypedWrapper(const Buffer<U, M, T>& buffer, usize size, usize byte_offset) : Buff(buffer, size * sizeof(value_type), byte_offset) {
		}


		usize size() const {
			return this->byte_size() / sizeof(value_type);
		}

		usize offset() const {
			if constexpr(is_sub) {
				return this->byte_offset() / sizeof(value_type);
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

		template<typename Buff>
		explicit TypedMapping(const TypedWrapper<Elem, Buff>& buffer) : Mapping(buffer) {
			static_assert(Buff::memory_type == MemoryType::CpuVisible);
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

#endif // YAVE_GRAPHICS_BUFFERS_TYPEDWRAPPER_H
