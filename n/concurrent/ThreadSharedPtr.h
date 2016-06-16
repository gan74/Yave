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

#ifndef N_CONCURRENT_THREADSHAREDPTR_H
#define N_CONCURRENT_THREADSHAREDPTR_H

#include "HazardPtr.h"
#include "DetachedThreadPtr.h"

namespace n {
namespace concurent {

template<typename T>
class ThreadSharedPtr
{
	public:
		ThreadSharedPtr(T *&&p) : ptr(p) {
		}

		operator HazardPtr<T>() const {
			return HazardPtr<T>(ptr);
		}

		operator DetachedThreadPtr<T>() {
			return DetachedThreadPtr<T>(HazardPtr<T>(ptr));
		}

		bool operator=(const DetachedThreadPtr<T> &p) {
			T *o = const_cast<T *>(p.original);
			bool r = ptr.compare_exchange_strong(o, p.obj);
			if(r) {
				HazardPtr<T>::deleteLater(o);
			}
			return r;
		}

	private:
		AtomicPtr<T> ptr;
};

}
}

#endif // N_CONCURRENT_THREADSHAREDPTR_H

