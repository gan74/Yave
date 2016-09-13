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
#ifndef YAVE_BUFFER_BUFFERMEMORYREFERENCE_H
#define YAVE_BUFFER_BUFFERMEMORYREFERENCE_H

#include "GenericBuffer.h"

namespace yave {

template<MemoryFlags Flags, BufferTransfer Transfer>
class BufferMemoryReference {

	public:
		template<BufferUsage Usage>
		BufferMemoryReference(const GenericBuffer<Usage, Flags, Transfer>& buffer) : _base(&buffer) {
		}

		BufferMemoryReference() = default;

		const BufferBase& get_buffer() const {
			return *_base;
		}

		const BufferBase* operator->() const {
			return _base;
		}

	private:
		NotOwned<const BufferBase*> _base;

};

}
#endif // YAVE_BUFFER_BUFFERMEMORYREFERENCE_H
