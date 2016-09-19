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
#ifndef YAVE_BUFFER_BUFFERREFERENCE_H
#define YAVE_BUFFER_BUFFERREFERENCE_H

#include <yave/yave.h>

#include <y/core/Ptr.h>
#include "TypedBuffer.h"

namespace yave {

#if 0

template<BufferUsage Usage>
class BufferReference {

	Y_TODO(We never use the buffer !!)

	public:
		template<typename T, MemoryFlags Flags>
		BufferReference(const TypedBuffer<T, Usage, Flags>& buffer) :
				_info(buffer.descriptor_info()),
				_ref(&buffer) {
		}

		/*template<typename T, MemoryFlags Flags>
		BufferReference(TypedBuffer<T, Usage, Flags>&& buffer) : BufferReference(core::rc(std::move(buffer))) {
		}*/

		BufferReference() {
		}

		auto descriptor_info() const {
			return _info;
		}

	private:
		vk::DescriptorBufferInfo _info;
		NotOwned<const Buffer<Usage>*> _ref;
};

#endif

}

#endif // YAVE_BUFFERREFERENCE_H
