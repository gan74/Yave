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

#include "Thread.h"
#include <pthread.h>
#include <n/core/Timer.h>

#if defined WIN32 || defined _WIN32 || defined __CYGWIN__
#include <windows.h>
#define tsleep(sec) Sleep((sec) * 1000)
#endif
#ifdef Linux
#include <unistd.h>
#define tsleep(sec) sleep((sec))
#endif

namespace n {
namespace concurent {

thread_local Thread *Thread::self = 0;

Thread::Thread() : handle(new pthread_t), running(false), toDelete(false), timer(0) {
}

Thread::~Thread() {
	join();
	delete (pthread_t *)handle;
	if(timer) {
		delete timer;
	}
}

Thread *Thread::getCurrent() {
	return self;
}

bool Thread::isRunning() const {
	return running;
}

bool Thread::start() {
	return !pthread_create((pthread_t *)handle, 0, Thread::threadCreate, this);
}

void Thread::kill() {
	pthread_cancel(*(pthread_t *)handle);
}

void Thread::join() const {
	if(!running) {
		return;
	}
	pthread_join(*(pthread_t *)handle, 0);
}

void Thread::deleteLater() {
	toDelete = true;
}

bool Thread::willBeDeleted() const {
	return toDelete;
}

void Thread::sleep(double sec) {
	tsleep(sec);
}

void *Thread::threadCreate(void *arg) {
	self = (Thread *)arg;
	if(!self->timer) {
		self->timer = new core::Timer();
	} else {
		self->timer->reset();
	}
	self->running = true;
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
	self->run();
	self->running = false;
	pthread_exit(0);
	if(self->willBeDeleted()) {
		delete self;
	}
	return 0;
}

}
}
