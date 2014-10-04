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

#ifndef N_CONCURENT_MUTEX_H
#define N_CONCURENT_MUTEX_H

#include <pthread.h>

namespace n {
namespace concurent {

class Thread;

class Signal
{
	public:
		Signal();

		Signal(const Signal &) = delete;

		void notify();
		void notifyAll();

	private:
		friend class Mutex;
		pthread_cond_t signal;
};

class Mutex
{
	public:
		Mutex();

		void lock();
		void unlock();
		bool trylock();

		void wait() { wait(signal); }
		bool wait(double sec)  { return wait(sec, signal); }
		void notify() { notify(signal); }
		void notifyAll() { notifyAll(signal); }

		void wait(Signal &sig);
		bool wait(double sec, Signal &sig);
		void notify(Signal &sig);
		void notifyAll(Signal &sig);

	private:
		void checkLockedByThread() const;

		pthread_mutex_t mutex;
		Signal signal;
		Thread *thread;
};

}
}

#endif // N_CONCURENT_MUTEX_H
