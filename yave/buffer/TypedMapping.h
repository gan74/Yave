/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/
#ifndef YAVE_BUFFER_TYPEDMAPPING_H
#define YAVE_BUFFER_TYPEDMAPPING_H

#include "BufferUsage.h"
#include "StagingBufferMapping.h"

namespace yave {

template<MemoryFlags Flags>
using MemoryMapping = typename std::conditional<is_cpu_visible<Flags>(), CpuVisibleMapping, StagingBufferMapping>::type;

template<typename Elem, MemoryFlags Flags>
class TypedMapping : public MemoryMapping<Flags> {

	using Base = MemoryMapping<Flags>;
	public:
		using iterator = Elem* ;
		using const_iterator = Elem const* ;
		using Element = Elem;

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
			return iterator(this->data());
		}

		iterator end() {
			return begin() + size();
		}

		const_iterator begin() const {
			return iterator(this->data());
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

		Element& operator[](usize i) {
			return begin()[i];
		}

		const Element& operator[](usize i) const{
			return begin()[i];
		}

	private:
		void swap(TypedMapping& other) {
			Base::swap(other);
		}

};


}

#endif // YAVE_BUFFER_TYPEDMAPPING_H
