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

#ifndef N_CONCURENT_THREAD_H
#define N_CONCURENT_THREAD_H

#include <n/utils.h>
#include <pthread.h>

namespace n {

namespace core {
class Timer;
}

namespace concurent {

enum RecursionMode
{
	Recursive,
	NonRecursive
};


class Thread : core::NonCopyable
{
	public:
		Thread();

		virtual ~Thread();

		static Thread *getCurrent();

		bool isRunning() const;
		bool start();
		void kill();
		void join() const;
		void deleteLater();
		bool willBeDeleted() const;
		static void sleep(double sec);

	protected:
		virtual void run() = 0;

	private:
		static void *threadCreate(void *arg);

		static thread_local Thread *self;
		pthread_t handle;
		bool running;
		bool toDelete;
		core::Timer *timer;
};

}
}

#endif // N_CONCURENT_THREAD_H
