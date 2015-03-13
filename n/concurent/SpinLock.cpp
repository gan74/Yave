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

#include "SpinLock.h"

#ifdef N_USE_PTHREAD_SPINLOCK
#include <pthread.h>
#endif

namespace n {
namespace concurent {

SpinLock::SpinLock() {
	#ifndef N_USE_PTHREAD_SPINLOCK
	spin = false;
	#else
	#wrning SpinLokcs are a pthread extension !
	spin = malloc(sizeof(pthread_spinlock_t));
	pthread_spin_init((pthread_spinlock_t *)spin, PTHREAD_PROCESS_PRIVATE);
	#endif
}

SpinLock::SpinLock(SpinLock &&s) {
	#ifndef N_USE_PTHREAD_SPINLOCK
	spin = s.spin;
	#else
	spin = s.spin;
	s.spin = 0;
	#endif
}

SpinLock::~SpinLock() {
	#ifdef N_USE_PTHREAD_SPINLOCK
	if(spin) {
		pthread_spin_destroy((pthread_spinlock_t *)spin);
		free(spin);
	}
	#endif
}

void SpinLock::lock() {
	#ifdef N_USE_PTHREAD_SPINLOCK
	pthread_spin_lock((pthread_spinlock_t *)spin);
	#else
	while(spin.load(std::memory_order_acquire) || spin.exchange(true, std::memory_order_acquire)) {
	}
	#endif
}

bool SpinLock::trylock() {
	#ifdef N_USE_PTHREAD_SPINLOCK
	if(!pthread_spin_trylock((pthread_spinlock_t *)spin)) {
		return true;
	}
	return false;
	#else
	return !spin.load(std::memory_order_acquire) && !spin.exchange(true, std::memory_order_acquire);
	#endif
}

void SpinLock::unlock() {
	#ifdef N_USE_PTHREAD_SPINLOCK
	pthread_spin_unlock((pthread_spinlock_t *)spin);
	#else
	spin.store(false, std::memory_order_release);
	#endif
}

}
}
