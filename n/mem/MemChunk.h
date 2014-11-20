/*******************************
Copyright (C) 2013-2014 gr�goire ANGERAND

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

#ifndef N_MEM_MEMCHUNK_H
#define N_MEM_MEMCHUNK_H

#include <n/utils.h>

namespace n {
namespace mem {

template<uint US>
class MemChunk : public NonCopyable
{
	union InternalType
	{
		byte obj[US];
		uint ptr;
	};

	public:
		MemChunk(uint s) : ptr(0), size(s), buffer(new InternalType[size]) {
			for(uint i = 0; i != size; i++) {
				buffer[i].ptr = i + 1;
			}
			buffer[size - 1].ptr = (uint)-1;
		}

		void *alloc() {
			if(ptr >= size) {
				return 0;
			}
			uint prev = ptr;
			ptr = buffer[ptr].ptr;
			return buffer[prev].obj;
		}

		void free(void *p) {
			free((InternalType *)p);
		}

	private:
		void free(InternalType *p) {
			if(!p) {
				return;
			}
			if(p >= buffer && p < buffer + size) {
				buffer[p - buffer].ptr = ptr;
				ptr	= p - buffer;
			} else {
				delete p;
			}
		}

		uint ptr;
		const uint size;
		InternalType *buffer;
};

}
}


#endif // N_MEM_MEMCHUNK_H
