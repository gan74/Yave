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
#ifndef YAVE_BUFFER_GENERICBUFFER_H
#define YAVE_BUFFER_GENERICBUFFER_H

#include "buffer.h"

#include "BufferBase.h"

namespace yave {

template<BufferUsage Usage>
class Buffer : public BufferBase {
	protected:
		using BufferBase::BufferBase;
};

template<BufferUsage Usage, MemoryFlags Flags = PreferedMemoryFlags<Usage>::value, BufferTransfer Transfer = PreferedBufferTransfer<Flags>::value>
class GenericBuffer : public Buffer<Usage> {

	static_assert(Usage != BufferUsage::None || Transfer != BufferTransfer::None, "GenericBuffer should not have Usage == BufferUsage::None");

	using Base = Buffer<Usage>;

	public:
		GenericBuffer() = default;

		GenericBuffer(DevicePtr dptr, usize byte_size) : Base(dptr, byte_size, Usage, Flags, Transfer) {
		}

		GenericBuffer(GenericBuffer&& other) : GenericBuffer() {
			swap(other);
		}

		GenericBuffer& operator=(GenericBuffer&& other) {
			swap(other);
			return *this;
		}

	protected:
		void swap(GenericBuffer& other) {
			Base::swap(other);
		}
};


}

#endif // YAVE_BUFFER_GENERICBUFFER_H
