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
#ifndef Y_CONCURRENT_LOCKGUARD_H
#define Y_CONCURRENT_LOCKGUARD_H

#include <y/utils.h>

namespace y {
namespace concurrent {

// std::lock_guard is not even movable ?
template<typename Lock>
class LockGuard : NonCopyable {

	public:
		LockGuard(Lock& lock) : _lock(&lock) {
			lock.lock();
		}

		LockGuard(LockGuard&& other) {
			std::swap(_lock, other._lock);
		}

		~LockGuard() {
			if(_lock) {
				_lock->unlock();
			}
		}

	private:
		LockGuard() = default;

		NotOwned<Lock*> _lock;

};


template<typename T>
auto lock(T& t) {
	return LockGuard<T>(t);
}

}
}



#endif // Y_CONCURRENT_LOCKGUARD_H
