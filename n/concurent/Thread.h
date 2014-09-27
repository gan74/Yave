/*******************************
Copyright (C) 2009-2010 grégoire ANGERAND

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

namespace n {

namespace core {
class Timer;
}

namespace concurent {


class Thread
{
	public:
		Thread();
		Thread(const Thread &) = delete;

		virtual ~Thread();

		virtual void run() = 0;

		static Thread *getCurrent();

		bool isRunning() const;
		bool start();
		void kill();
		void join() const;
		void deleteLater();
		bool willBeDeleted() const;
		static void sleep(double sec);


	private:
		static void *threadCreate(void *arg);

		static thread_local Thread *self;
		void *handle;
		bool running;
		bool toDelete;
		core::Timer *timer;
};

}
}

#endif // N_CONCURENT_THREAD_H
