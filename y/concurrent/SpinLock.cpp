/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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
#include <thread>

namespace y {
namespace concurrent {

SpinLock::SpinLock() : _spin(ATOMIC_FLAG_INIT) {
}

SpinLock::~SpinLock() {
}

void SpinLock::lock() {
	for(usize failed = 0; !try_lock(); failed++) {
		if(failed > yield_threshold) {
			std::this_thread::yield();
		}
	}

}

bool SpinLock::try_lock() {
	return !_spin.test_and_set(std::memory_order_acquire);

}

void SpinLock::unlock() {
	_spin.clear();
}

}
}
