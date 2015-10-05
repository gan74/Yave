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
#include "Mutex.h"
#include <unistd.h>

namespace n {
namespace concurrent {

thread_local Thread *Thread::self = 0;

struct Thread::Internal
{
	pthread_t handle;
	Mutex join;
};

Thread::Thread() : internal(new Internal), toDelete(false) {
}

Thread::~Thread() {
	delete internal;
}

Thread *Thread::getCurrent() {
	return self;
}

bool Thread::isRunning() const {
	if(internal->join.trylock()) {
		internal->join.unlock();
		return false;
	}
	return true;
}

bool Thread::start() {
	internal->join.lock();
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if(!pthread_create(&internal->handle, &attr, Thread::createThread, this)) {
		pthread_attr_destroy(&attr);
		return true;
	}
	pthread_attr_destroy(&attr);
	internal->join.unlock();
	return false;
}

void Thread::join() const {
	#ifdef N_DEBUG
	if(willBeDeleted()) {
		fatal("Auto deleted threads should not be joined");
	}
	#endif
	internal->join.lock();
	internal->join.unlock();
}

void Thread::deleteLater() {
	toDelete = true;
}

bool Thread::willBeDeleted() const {
	return toDelete;
}

void Thread::sleep(double sec) {
	usleep(sec * 1000000);
}

void *Thread::createThread(void *arg) {
	self = reinterpret_cast<Thread *>(arg);
	{ HazardPtr<int>(0); } // init Hazard stuffs
	self->run();
	self->internal->join.unlock();
	internal::closeThreadHazards();
	if(self->willBeDeleted()) {
		delete self;
	}
	return 0;
}

}
}
