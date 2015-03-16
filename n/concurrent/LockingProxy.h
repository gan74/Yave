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

#ifndef N_CONCURENT_LOCKINGPROXY_H
#define N_CONCURENT_LOCKINGPROXY_H

#include "Mutex.h"

namespace n {
namespace concurent {

template<typename T>
class LockingProxy
{
	public:
		LockingProxy(T *obj) : ptr(obj) {
			if(ptr) {
				ptr->lock();
			}
		}

		~LockingProxy() {
			if(ptr) {
				ptr->unlock();
			}
		}

		T *operator->() const {
			return ptr;
		}

	private:
		T *ptr;
};

}
}

#endif // N_CONCURENT_LOCKINGPROXY_H
