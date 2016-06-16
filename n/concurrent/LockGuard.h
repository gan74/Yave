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

#ifndef N_CONCURRENT_LOCKGUARD_H
#define N_CONCURRENT_LOCKGUARD_H

#include <n/types.h>

namespace n {
namespace concurrent {

template<typename T>
class LockGuard : NonCopyable
{
	public:
		LockGuard(T &t) : lock(&t) {
			t.lock();
		}

		LockGuard(LockGuard<T> &&l) : lock(0) {
			std::swap(lock, l.lock);
		}

		~LockGuard() {
			if(lock) {
				lock->unlock();
			}
		}

	private:
		T *lock;
};

template<typename T>
LockGuard<T> lockGuard(T &t) {
	return LockGuard<T>(t);
}

#define N_LOCK_LINE_HELPER(LOCK, LINE) auto _lockGuard_at_ ## LINE = n::concurrent::lockGuard(LOCK)
#define N_LOCK_HELPER(LOCK, LINE) N_LOCK_LINE_HELPER(LOCK, LINE)
#define N_LOCK(LOCK) N_LOCK_HELPER(LOCK, __LINE__)
}
}


#endif // N_CONCURRENT_LOCKGUARD_H

