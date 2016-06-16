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

#ifndef N_MEM_FIXEDSIZEALLOCATOR_H
#define N_MEM_FIXEDSIZEALLOCATOR_H

#include "Region.h"
#include <n/core/List.h>

namespace n {
namespace mem {

class FixedSizeAllocator : NonCopyable
{
	public:
		FixedSizeAllocator(uint s) : size(toAllocableSize(s)) {
		}

		~FixedSizeAllocator() {
			for(Region *reg : regs) {
				delete reg;
			}
		}

		void *allocate() {
			for(core::List<Region *>::iterator it = regs.begin(); it != regs.end(); ++it) {
				Region *reg = *it;
				void *ptr = reg->allocate();
				if(ptr) {
					regs.move(it, regs.begin());
					return ptr;
				}
			}

			regs.prepend(new Region(size));
			return regs.first()->allocate();
		}

		bool desallocate(void *ptr) {
			if(!ptr) {
				return true;
			}
			for(core::List<Region *>::iterator it = regs.begin(); it != regs.end(); ++it) {
				Region *reg = *it;
				if(reg->desallocate(ptr)) {
					if(reg->isEmpty()) {
						regs.remove(it);
						delete reg;
					} else if(it != regs.begin()) {
						regs.move(it, regs.begin());
					}
					return true;
				}
			}
			return false;
		}


	private:
		uint toAllocableSize(uint s) {
			uint m = s % sizeof(void *);
			return m ? s + sizeof(void *) - m : s;
		}

		const uint size;
		core::List<Region *> regs;

};

}
}


#endif // N_MEM_FIXEDSIZEALLOCATOR_H

