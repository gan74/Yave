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

#ifndef N_MEM_ALLOCATOR_H
#define N_MEM_ALLOCATOR_H


#include "FixedSizeAllocator.h"
#include <n/core/List.h>

namespace n {
namespace mem {

class Allocator : core::NonCopyable
{
	public:
		Allocator() {
			for(uint i = 0; i != regs; i++)	{
				allocTable[i] = new FixedSizeAllocator(toSize(i));
			}
		}

		~Allocator() {
			for(uint i = 0; i != regs; i++) {
				delete allocTable[i];
			}
		}

		void *allocate(uint size) {
			uint i = toIndex(size);
			if(i < regs) {
				return allocTable[i]->allocate();
			}
			return malloc(size);
		}

		void desallocate(void *ptr) {
			for(uint i = 0; i != regs; i++) {
				if(allocTable[i]->desallocate(ptr)) {
					return;
				}
			}
			free(ptr);
		}

		void desallocate(void *ptr, uint size) {
			uint i = toIndex(size);
			if(allocTable[i]->desallocate(ptr)) {
				return;
			}
			desallocate(ptr);
		}

		/*void *operator()(uint size) {
			return allocate(size);
		}

		void operator()(void *ptr) {
			return desallocate(ptr);
		}

		void operator()(void *ptr, uint size) {
			return desallocate(ptr, size);
		}*/

	private:
		static constexpr uint regs = 12;
		static constexpr uint logBytes = core::log2ui(sizeof(void *));

		uint toIndex(uint size) {
			size = std::min(size, sizeof(void *));
			uint l2 = core::log2ui(size);
			l2 += (1u << l2) < size;
			return l2 - logBytes;
		}

		uint toSize(uint index) {
			return sizeof(void *) << index;
		}

		FixedSizeAllocator *allocTable[regs];
};



}
}

#endif // N_MEM_ALLOCATOR_H
