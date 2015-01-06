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
#include "Thread.h"
#include "HazardPtr.h"
#include <pthread.h>

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

struct Thread::Internal
{
	pthread_t handle;
};

Thread::Thread() : internal(new Internal), running(false), toDelete(false) {
}

Thread::~Thread() {
	join();
}

Thread *Thread::getCurrent() {
	return self;
}

bool Thread::isRunning() const {
	return running;
}

bool Thread::start() {
	return !pthread_create(&internal->handle, 0, Thread::threadCreate, this);
}

void Thread::kill() {
	pthread_cancel(internal->handle);
}

void Thread::join() const {
	pthread_join(internal->handle, 0);
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
	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
	{ HazardPtr<int>(0); } // init Hazard stuffs
	self->running = true;
	self->run();
	self->running = false;
	internal::closeThreadHazards();
	if(self->willBeDeleted()) {
		delete self;
	}
	pthread_exit(0);
	return 0;
}

}
}
