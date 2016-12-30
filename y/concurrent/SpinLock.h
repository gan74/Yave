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

#ifndef Y_CONCURRENT_SPINLOCK_H
#define Y_CONCURRENT_SPINLOCK_H

#include <y/utils.h>
#include <atomic>

namespace y {
namespace concurrent {

class SpinLock : NonCopyable {

	static constexpr usize yield_threshold = usize(-1);

	using Type = bool;
	static constexpr Type Locked = true;
	static constexpr Type Unlocked = false;

	public:
		SpinLock();
		~SpinLock();

		void lock();
		void unlock();
		bool try_lock();

	private:
		//std::atomic_flag _spin;
		std::atomic<Type> _spin;
};


}
}

#endif // Y_CONCURRENT_SPINLOCK_H
