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

#include "Mutex.h"

#include <ctime>
#include <n/defines.h>

namespace n {
namespace concurent {

WaitCondition::WaitCondition() : condition(PTHREAD_COND_INITIALIZER) {
}

void WaitCondition::notify() {
	pthread_cond_signal(&condition);
}

void WaitCondition::notifyAll() {
	pthread_cond_broadcast(&condition);
}

Mutex::Mutex(RecursionMode r) : mutex(r == NonRecursive ?  PTHREAD_MUTEX_INITIALIZER : PTHREAD_RECURSIVE_MUTEX_INITIALIZER)
#ifdef N_DEBUG
, thread(0)
#endif
{
}

Mutex::Mutex(Mutex &&m) : mutex(m.mutex) {
	m.mutex = 0;
}

void Mutex::lock() {
	pthread_mutex_lock(&mutex);
	#ifdef N_DEBUG
	thread = Thread::getCurrent();
	#endif
}

bool Mutex::trylock() {
	if(!pthread_mutex_trylock(&mutex)) {
		#ifdef N_DEBUG
		thread = Thread::getCurrent();
		#endif
		return true;
	}
	return false;
}

void Mutex::unlock() {
	#ifdef N_DEBUG
	thread = 0;
	#endif
	pthread_mutex_unlock(&mutex);
}

void Mutex::wait(WaitCondition &cond) {
	checkLockedByThread();
	pthread_cond_wait(&(cond.condition), &mutex);
}

bool Mutex::wait(double sec, WaitCondition &cond) {
	checkLockedByThread();
	timespec timer;
	timer.tv_nsec = time(0) + (long)(sec * 1000000000);
	int err = pthread_cond_timedwait(&(cond.condition), &mutex, &timer);
	if(err && err == ETIMEDOUT) {
		return true;
	}
	return false;
}

void Mutex::notify(WaitCondition &cond) {
	cond.notify();
}

void Mutex::notifyAll(WaitCondition &cond) {
	cond.notifyAll();
}

void Mutex::checkLockedByThread() const {
	#ifdef N_DEBUG
	if(Thread::getCurrent() != thread) {
		fatal("Waiting on an unlocked mutex");
	}
	#endif
}


}
}
