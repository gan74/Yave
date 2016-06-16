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

#ifndef N_MEM_REGION_H
#define N_MEM_REGION_H

#include <n/utils.h>

namespace n {
namespace mem {

class Region : NonCopyable
{
	public:
		Region(uint blckSize, uint nBlck = 0) : blockSize(blckSize), size(nBlck ? nBlck : std::max(16, 1 << (16 - log2ui(blckSize)))), left(size), mem(malloc(blockSize * size)), ptr(mem) {
			byte *it = (byte *)mem;
			void **last = (void **)ptr;
			for(uint i = 0; i != size; i++, it += blockSize) {
				*last = it;
				last = (void **)it;
			}
			*last = 0;
		}

		~Region() {
			if(!isEmpty()) {
				fatal("Deleting non-empty memory region.");
			}
			free(mem);
		}

		void *allocate() {
			if(!ptr) {
				return 0;
			}
			void **addr = (void **)ptr;
			ptr = *addr;
			left--;
			return addr;
		}

		bool desallocate(void *p) {
			if(!contains(p)) {
				return false;
			}
			*((void **)p) = ptr;
			ptr = p;
			left++;
			return true;
		}

		uint getFreeCapacity() const {
			return left;
		}

		bool isEmpty() const {
			return left == size;
		}

		bool contains(void *p) const {
			return p >= mem && p <= ((byte *)mem + blockSize * size);
		}

	private:
		uint blockSize;
		uint size;
		uint left;
		void *mem;
		void *ptr;

};

}
}


#endif // N_MEM_REGION_H

