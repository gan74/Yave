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

#ifndef N_CONCURRENT_SYNCHRONIZEDQUEUE_H
#define N_CONCURRENT_SYNCHRONIZEDQUEUE_H

#include <n/core/List.h>
#include <n/core/Option.h>
#include "Mutex.h"

namespace n {
namespace concurrent {

template<typename T>
class SynchronizedQueue
{
	public:
		SynchronizedQueue() {
		}

		void queue(const T &e) {
			mutex.lock();
			list.append(e);
			mutex.notify();
			mutex.unlock();
		}

		T dequeue() {
			mutex.lock();
			while(list.isEmpty()) {
				mutex.wait();
			}
			T e = list.first();
			list.popFront();
			mutex.unlock();
			return e;
		}

		core::Option<T> tryDequeue() {
			if(!mutex.trylock() ) {
				return core::Option<T>();
			}
			if(list.isEmpty()) {
				mutex.unlock();
				return core::Option<T>();
			}
			T e = list.first();
			list.popFront();
			mutex.unlock();
			return e;
		}

		core::List<T> getDetachedList() const {
			core::List<T> l;
			mutex.lock();
			l = list;
			mutex.unlock();
			return l;
		}

		core::List<T> getAndClear() {
			core::List<T> l;
			mutex.lock();
			list.swap(l);
			mutex.unlock();
			return l;
		}

		uint size() const {
			return list.size();
		}

	private:
		mutable Mutex mutex;
		core::List<T> list;
};

}
}

#endif // N_CONCURRENT_SYNCHRONIZEDQUEUE_H
