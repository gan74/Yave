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

#ifndef N_CORE_SMALLOBJECTALLOCATOR_H
#define N_CORE_SMALLOBJECTALLOCATOR_H

#include <n/types.h>

namespace n {
namespace core {

template<typename T, uint S = 1024>
class SmallObjectAllocator
{
	union InternalType
	{
		T obj;
		uint ptr;
	};

	public:
		SmallObjectAllocator() : ptr(0) {
			for(uint i = 0; i != S; i++) {
				buffer[i].ptr = i + 1;
			}
			buffer[S - 1].ptr = (uint)-1;
		}

		T *alloc() {
			if(ptr >= S) {
				return new uint;
			}
			uint prev = ptr;
			ptr = buffer[ptr].ptr;
			return &buffer[prev].obj;
		}

		void free(T *p) {
			free((InternalType *)p);
		}

	private:
		void free(InternalType *p) {
			if(!p) {
				return;
			}
			if(p >= buffer && p < buffer + S) {
				buffer[p - buffer].ptr = ptr;
				ptr	= p - buffer;
			} else {
				delete p;
			}
		}

		uint ptr;
		InternalType buffer[S];
};

}
}


#endif // N_CORE_SMALLOBJECTALLOCATOR_H
