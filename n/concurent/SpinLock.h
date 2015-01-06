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

#ifndef N_CONCURENT_SPINLOCK_H
#define N_CONCURENT_SPINLOCK_H

#include <n/utils.h>
#include "Atomic.h"

namespace n {
namespace concurent {

class SpinLock : core::NonCopyable
{
	public:
		SpinLock();
		~SpinLock();

		void lock();
		void unlock();
		bool trylock();

	private:
		#ifdef N_USE_PTHREAD_SPINLOCK
		void *spin;
		#else
		volatile abool spin;
		#endif
};


}
}

#endif // N_CONCURENT_SPINLOCK_H
