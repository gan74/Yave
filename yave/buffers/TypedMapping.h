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
#ifndef YAVE_BUFFERS_TYPEDMAPPING_H
#define YAVE_BUFFERS_TYPEDMAPPING_H

#include "BufferUsage.h"
#include "StagingBufferMapping.h"

namespace yave {

template<MemoryFlags Flags>
using MemoryMapping = std::conditional_t<is_cpu_visible(Flags), CpuVisibleMapping, StagingBufferMapping>;

template<typename Elem, MemoryFlags Flags>
class TypedMapping : public MemoryMapping<Flags> {

	using Base = MemoryMapping<Flags>;
	public:
		using iterator = Elem* ;
		using const_iterator = Elem const* ;
		using value_type = Elem;

		template<typename T>
		TypedMapping(T& buffer) : Base(buffer) {
		}

		TypedMapping(TypedMapping&& other) {
			swap(other);
		}

		TypedMapping& operator=(TypedMapping&& other) {
			swap(other);
			return *this;
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

	private:
		void swap(TypedMapping& other) {
			Base::swap(other);
		}

};


}

#endif // YAVE_BUFFERS_TYPEDMAPPING_H
