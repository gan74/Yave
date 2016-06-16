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

#ifndef N_CONCURRENT_DETACHEDTHREADPTR_H
#define N_CONCURRENT_DETACHEDTHREADPTR_H

#include "HazardPtr.h"
#include <n/utils.h>

namespace n {
namespace concurent {

template<typename T>
class DetachedThreadPtr : NonCopyable
{
	public:
		DetachedThreadPtr(DetachedThreadPtr<T> &&p) : original(p.original), obj(p.obj) {
		}

		T &operator*() {
			return *obj;
		}

		const T &operator*() const {
			return *obj;
		}

		T *operator->() {
			return *obj;
		}

		const T *operator->() const {
			return *obj;
		}

	private:
		template<typename U>
		friend class ThreadSharedPtr;

		DetachedThreadPtr(HazardPtr<T> &&ptr) : original(ptr), obj(new T(*ptr)) {
		}

		const T *original;
		T *obj;
};

}
}

#endif // N_CONCURRENT_DETACHEDTHREADPTR_H

