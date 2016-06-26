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

#ifndef N_CORE_PTR_H
#define N_CORE_PTR_H

#include <n/utils.h>


namespace n {
namespace core {

template<typename T>
class Ptr : NonCopyable
{
	public:
		Ptr(T *&&p = 0) : ptr(p) {
		}

		~Ptr() {
			delete ptr;
		}

		const T &operator*() const {
			return *ptr;
		}

		T &operator*() {
			return *ptr;
		}

		T const *operator->() const {
			return ptr;
		}

		T *operator->() {
			return ptr;
		}

		/*operator T *() {
			return ptr;
		}

		operator T const *() const{
			return ptr;
		}*/

		operator void *() {
			return ptr;
		}

		operator void const *() const {
			return ptr;
		}

		/*bool operator==(const T * const t) const {
			return t == ptr;
		}

		bool operator!=(const T * const t) const {
			return t != ptr;
		}*/

		bool operator<(const T * const t) const {
			return ptr < t;
		}

		bool operator>(const T * const t) const {
			return ptr > t;
		}

		bool operator!() const {
			return isNull();
		}

		/*explicit operator bool() const {
			return !isNull();
		}*/

		bool isNull() const {
			return !ptr;
		}

		T *getPtr() {
			return ptr;
		}

		T const *getPtr() const {
			return ptr;
		}

	protected:
		T *ptr;
};

template<typename T>
class NullCpyPtr : public Ptr<T>
{
	public:
		NullCpyPtr(T *&&p = 0) : Ptr<T>(p) {
		}

		NullCpyPtr(const NullCpyPtr<T> &p) : Ptr<T>(0) {
		}

		NullCpyPtr(NullCpyPtr<T> &&p) : Ptr<T>(p.ptr) {
			p.ptr = 0;
		}
};

}
}


#endif // N_CORE_PTR_H

