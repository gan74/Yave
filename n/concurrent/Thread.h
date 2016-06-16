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

#ifndef N_CONCURRENT_THREAD_H
#define N_CONCURRENT_THREAD_H

#include <n/utils.h>

namespace n {

namespace concurrent {

enum RecursionMode
{
	Recursive,
	NonRecursive
};

class Thread : NonCopyable
{
	struct Internal;

	public:
		Thread();

		virtual ~Thread();

		static Thread *getCurrent();
		static uint getCurrentId();

		bool isRunning() const;
		bool start();
		void join() const;
		void deleteLater();
		bool willBeDeleted() const;
		static void sleep(double sec);

	protected:
		virtual void run() = 0;

	private:
		static void *createThread(void *arg);

		static thread_local Thread *self;
		static thread_local uint currentId;

		Internal *internal;
		bool toDelete;
		uint id;
};

}
}

#endif // N_CONCURRENT_THREAD_H
