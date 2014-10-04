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

#include "Mutex.h"
#include "Thread.h"
#include <ctime>
#include <n/defines.h>
#include <iostream>

namespace n {
namespace concurent {

Signal::Signal() : signal(PTHREAD_COND_INITIALIZER) {
}

void Signal::notify() {
	pthread_cond_signal(&signal);
}

void Signal::notifyAll() {
	pthread_cond_broadcast(&signal);
}


Mutex::Mutex() : mutex(PTHREAD_MUTEX_INITIALIZER), thread(0) {
}

void Mutex::lock() {
	pthread_mutex_lock(&mutex);
	thread = Thread::getCurrent();
}

bool Mutex::trylock() {
	if(!pthread_mutex_trylock(&mutex)) {
		thread = Thread::getCurrent();
		return true;
	}
	return false;
}

void Mutex::unlock() {
	thread = 0;
	pthread_mutex_unlock(&mutex);
}

void Mutex::wait(Signal &sig) {
	checkLockedByThread();
	pthread_cond_wait(&(sig.signal), &mutex);
}

bool Mutex::wait(double sec, Signal &sig) {
	checkLockedByThread();
	timespec timer;
	timer.tv_nsec = time(0) + (long)(sec * 1000000000);
	int err = pthread_cond_timedwait(&(sig.signal), &mutex, &timer);
	if(err && err == ETIMEDOUT) {
		return true;
	}
	return false;
}

void Mutex::notify(Signal &sig) {
	sig.notify();
}

void Mutex::notifyAll(Signal &sig) {
	sig.notifyAll();
}

void Mutex::checkLockedByThread() const {
	if(!thread || Thread::getCurrent() != thread) {
		nError("Locking unlocked mutex");
	}
}


}
}
