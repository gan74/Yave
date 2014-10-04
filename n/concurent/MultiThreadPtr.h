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

#ifndef N_CONCURENT_MULTITHREADPTR_H
#define N_CONCURENT_MULTITHREADPTR_H

#include "Mutex.h"
#include <n/core/SmartPtr.h>

namespace n {
namespace concurent {

class PtrLock
{
	public:
		Mutex *getMutex() {
			return mutex;
		}

	protected:
		PtrLock() : mutex(0) {
		}

		PtrLock(const PtrLock &p) : mutex(0) {
			ref(p);
		}

		void ref(const PtrLock &p) {
			mutex = p.mutex;
		}

		void swap(PtrLock &p) {
			Mutex *m = mutex;
			mutex = p.mutex;
			p.mutex = m;
		}

		void unref(bool del) {
			if(mutex && del) {
				delete mutex;
			}
			mutex = 0;
		}

		void lock() {
			if(mutex) {
				mutex->lock();
			}
		}

		void unlock() {
			if(mutex) {
				mutex->unlock();
			}
		}

	private:
		Mutex *mutex;
};

template<typename T, typename Proxy = core::NoProxy<T>>
using MultiThreadPtr = core::SmartPtr<T, Proxy, PtrLock>;

}
}

#endif // N_CONCURENT_LOCKINGPTR_H
