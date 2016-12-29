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
#include "Buffer.h"

namespace yave {

template<BufferUsage Usage, MemoryFlags Flags = prefered_memory_flags<Usage>(), BufferTransfer Transfer = prefered_transfer<Flags>()>
class SubBuffer : public SubBufferBase {

	public:
		template<BufferUsage BuffUsage, typename Enable = typename std::enable_if<(BuffUsage & Usage) == Usage>::type>
		SubBuffer(const Buffer<BuffUsage, Flags, Transfer>& buffer, usize offset, usize len) : SubBufferBase(buffer, offset, len) {
		}

		template<BufferUsage BuffUsage>
		explicit SubBuffer(const Buffer<BuffUsage, Flags, Transfer>& buffer, usize offset = 0) : SubBuffer(buffer, offset, buffer.byte_size()) {
		}
};


}

#endif // YAVE_BUFFER_SUBBUFFER_H
