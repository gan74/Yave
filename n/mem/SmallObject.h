/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

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

#ifndef N_MEM_SMALLOBJECT_H
#define N_MEM_SMALLOBJECT_H

#include "Allocator.h"

namespace n {
namespace mem {

template<typename T>
class SmallObject
{
	public:
		N_FORCE_INLINE void *operator new(uint size) {
			return getAllocator()->allocate(size);
		}

		N_FORCE_INLINE void operator delete(void *ptr, uint size) {
			getAllocator()->desallocate(ptr, size);
		}

		template<typename... Args>
		static T *alloc(Args... args) {
			return new(getAllocator()->allocate(sizeof(T))) T(args...);
		}

		static void desalloc(T *ptr) {
			return getAllocator()->desallocate(ptr, sizeof(T));
		}

	private:
		static Allocator *getAllocator() {
			static Allocator *allocator = 0;
			return allocator ? allocator : allocator = new Allocator();
		}
};



}
}

#endif // N_MEM_SMALLOBJECT_H

