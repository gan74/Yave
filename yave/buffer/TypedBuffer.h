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
#ifndef YAVE_BUFFER_TYPEDBUFFER_H
#define YAVE_BUFFER_TYPEDBUFFER_H


#include <yave/yave.h>

#include "GenericBuffer.h"
#include "TypedMapping.h"

namespace yave {

template<typename Elem, BufferUsage Usage, MemoryFlags Flags = PreferedMemoryFlags<Usage>::value>
class TypedBuffer : public GenericBuffer<Usage, Flags> {
	public:
		using Element = Elem;

		TypedBuffer() : GenericBuffer<Usage, Flags>() {
		}

		TypedBuffer(DevicePtr dptr, const core::Vector<Elem>& data) : TypedBuffer(dptr, data.size()) {
			auto mapping = map();
			memcpy(mapping.data(), data.begin(), this->byte_size());
		}

		TypedBuffer(DevicePtr dptr, usize elem_count) : GenericBuffer<Usage, Flags>(dptr, elem_count *sizeof(Elem)) {
		}

		TypedBuffer(TypedBuffer&& other) {
			swap(other);
		}

		TypedBuffer& operator=(TypedBuffer&& other) {
			swap(other);
			return *this;
		}

		usize _size() const {
			return this->byte_size() / sizeof(Elem);
		}

		TypedMapping<Element, Flags> map() {
			return TypedMapping<Element, Flags>(*this);
		}

		vk::DescriptorBufferInfo descriptor_info() const {
			return vk::DescriptorBufferInfo()
					.setBuffer(this->get_vk_buffer())
					.setOffset(0)
					.setRange(_size())
				;
		}

	private:
		void swap(TypedBuffer& other) {
			GenericBuffer<Usage, Flags>::swap(other);
		}
};


}

#endif // YAVE_BUFFER_TYPEDBUFFER_H
