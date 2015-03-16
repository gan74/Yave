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
namespace concurrent {

SpinLock::SpinLock() : spin(new abool(false)) {
}

SpinLock::SpinLock(SpinLock &&s) {
	std::swap(s.spin, spin);

}

SpinLock::~SpinLock() {
	delete spin;
}

void SpinLock::lock() {
	while(spin->load(std::memory_order_acquire) || spin->exchange(true, std::memory_order_acquire)) {
	}
}

bool SpinLock::trylock() {
	return !spin->load(std::memory_order_acquire) && !spin->exchange(true, std::memory_order_acquire);
}

void SpinLock::unlock() {
	spin->store(false, std::memory_order_release);
}

}
}
