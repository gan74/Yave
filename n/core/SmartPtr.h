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

#ifndef N_CORE_SMARTPTR_H
#define N_CORE_SMARTPTR_H

#include <n/utils.h>
#include <n/concurrent/Atomic.h>
#include "Ptr.h"

namespace n {
namespace core {

template<typename T, typename C = concurrent::auint>
class SmartPtr : public Ptr<T>
{
	using Ptr<T>::ptr;
	public:
		SmartPtr(T *&&p = 0) : Ptr<T>(std::move(p)), count(new C(1)) {
		}

		SmartPtr(const SmartPtr<T, C> &p) : Ptr<T>(0), count(0) {
			ref(p);
		}

		~SmartPtr() {
			unref();
		}

		void swap(SmartPtr<T, C> &&p) {
			std::swap(ptr, p.ptr);
			std::swap(count, p.count);
		}

		uint getReferenceCount() const {
			return count ? uint(*count) : 0;
		}

		SmartPtr<T, C> &operator=(const SmartPtr<T, C> &p) {
			if(&p != this) {
				unref();
				ref(p);
			}
			return *this;
		}

		SmartPtr<T, C> &operator=(SmartPtr<T, C> &&p) {
			swap(std::move(p));
			return *this;
		}

	private:
		friend class SmartPtr<const T>;
		friend class SmartPtr<typename TypeInfo<T>::nonConst>;

		SmartPtr(T *p, C *c) : Ptr<T>(p), count(c) {
			++(*count);
		}

		void ref(const SmartPtr<T, C> &p) {
			if((count = p.count)) {
				(*count)++;
			}
			ptr = p.ptr;
		}

		void unref() {
			if(count && !--(*count)) {
				delete ptr;
				delete count;
			}
			ptr = 0;
			count = 0;
		}

		C *count;

};

}
}

#endif // SMARTPTR_H
