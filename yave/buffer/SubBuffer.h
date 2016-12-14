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
#ifndef YAVE_BUFFER_SUBBUFFER_H
#define YAVE_BUFFER_SUBBUFFER_H

#include "SubBufferBase.h"

namespace yave {

template<BufferUsage Usage, MemoryFlags Flags = PreferedMemoryFlags<Usage>::value, BufferTransfer Transfer = PreferedBufferTransfer<Flags>::value>
class SubBuffer : public SubBufferBase {

	using Base = SubBufferBase;

	public:
		SubBuffer() = default;

		SubBuffer(const GenericBuffer<Usage, Flags>& buffer, usize offset, usize len) : Base(buffer, offset, len) {
		}

		SubBuffer(const GenericBuffer<Usage, Flags>& buffer, usize offset = 0) : Base(buffer, offset, buffer.byte_size()) {
		}
};


}

#endif // YAVE_BUFFER_SUBBUFFER_H
