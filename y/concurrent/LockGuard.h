/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
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

		Lock* _lock;

};


template<typename T>
auto lock(T& t) {
	return LockGuard<T>(t);
}

}
}



#endif // Y_CONCURRENT_LOCKGUARD_H
