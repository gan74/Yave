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

#include "SpinLock.h"

namespace n {
namespace concurent {

SpinLock::SpinLock() : spin(PTHREAD_SPINLOCK_INITIALIZER) {
}

void SpinLock::lock() {
	pthread_spin_lock(&spin);
}

bool SpinLock::trylock() {
	if(!pthread_spin_trylock(&spin)) {
		return true;
	}
	return false;
}

void SpinLock::unlock() {
	pthread_spin_unlock(&spin);
}

}
}
