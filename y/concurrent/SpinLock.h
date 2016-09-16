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

#ifndef Y_CONCURRENT_SPINLOCK_H
#define Y_CONCURRENT_SPINLOCK_H

#include <y/utils.h>
#include <atomic>

namespace y {
namespace concurrent {

class SpinLock : NonCopyable {

	static constexpr usize yield_threshold = 1;
	public:
		SpinLock();
		~SpinLock();

		void lock();
		void unlock();
		bool try_lock();

	private:
		volatile std::atomic_flag _spin;
};


}
}

#endif // Y_CONCURRENT_SPINLOCK_H
