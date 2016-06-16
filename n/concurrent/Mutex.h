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

#ifndef N_CONCURRENT_MUTEX_H
#define N_CONCURRENT_MUTEX_H

#include "Thread.h"
#include <pthread.h>

namespace n {
namespace concurrent {

class WaitCondition
{
	public:
		WaitCondition();

		WaitCondition(const WaitCondition &) = delete;

		void notify();
		void notifyAll();

	private:
		friend class Mutex;
		pthread_cond_t condition;
};

class Mutex : NonCopyable
{
	public:
		Mutex(RecursionMode r = NonRecursive);

		Mutex(Mutex &&m);

		void lock();
		void unlock();
		bool trylock();

		void wait() { wait(condition); }
		bool wait(double sec)  { return wait(sec, condition); }
		void notify() { notify(condition); }
		void notifyAll() { notifyAll(condition); }

		void wait(WaitCondition &cond);
		bool wait(double sec, WaitCondition &cond);
		void notify(WaitCondition &cond);
		void notifyAll(WaitCondition &cond);

	private:
		void checkLockedByThread() const;

		pthread_mutex_t mutex;
		WaitCondition condition;

		#ifdef N_DEBUG
		Thread *thread;
		#endif
};


class RecursiveMutex : public Mutex
{
	public:
		RecursiveMutex() : Mutex(Recursive) {
		}
};

}
}

#endif // N_CONCURRENT_MUTEX_H
