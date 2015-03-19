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

#include <n/types.h>
#include <n/concurrent/Atomic.h>

namespace n {
namespace core {

template<typename T>
class NoProxy
{
	public:
		NoProxy(T *t) : obj(t) {
		}

		T *operator->() const {
			return obj;
		}

	private:
		T *obj;
};

template<typename T, typename C = concurrent::auint, typename Proxy = NoProxy<T>>
class SmartPtr
{
	public:
		SmartPtr(T *&&p = 0) : ptr(p), count(ptr ? new C(1) : 0) {
		}

		SmartPtr(const SmartPtr<T, C, Proxy> &p) : ptr(0), count(0) {
			ref(p);
		}

		~SmartPtr() {
			unref();
		}

		void swap(SmartPtr<T, C, Proxy> &p) {
			std::swap(ptr, p.ptr);
			std::swap(count, p.count);
		}

		bool isNull() const {
			return !ptr;
		}

		uint getReferenceCount() const {
			return count ? uint(*count) : 0;
		}

		T *pointer() {
			return ptr;
		}

		const T *pointer() const {
			return ptr;
		}

		T &operator*() const {
			return *ptr;
		}

		Proxy operator->() const {
			return Proxy(ptr);
		}

		SmartPtr<T, C, Proxy> &operator=(const SmartPtr<T, C, Proxy> &p) {
			unref();
			ref(p);
			return *this;
		}

		SmartPtr<T, C, Proxy> &operator=(SmartPtr<T, C, Proxy> &&p) {
			swap(p);
			return *this;
		}

		bool operator==(const T *p) const {
			return ptr == p;
		}

		bool operator!=(const T *p) const {
			return ptr != p;
		}

		bool operator<(const T *p) const {
			return ptr < p;
		}

		bool operator==(const SmartPtr<const T> &p) const {
			return ptr == p.ptr;
		}

		bool operator!=(const SmartPtr<const T> &p) const {
			return ptr != p.ptr;
		}

		bool operator<(const SmartPtr<const T> &p) const {
			return ptr < p.ptr;
		}

		bool operator!() const {
			return isNull();
		}

		explicit operator bool() const {
			return !isNull();
		}

		operator SmartPtr<const T>() const {
			return SmartPtr<const T>(ptr, count);
		}

	private:
		friend class SmartPtr<const T>;
		friend class SmartPtr<typename TypeInfo<T>::nonConst>;

		SmartPtr(T *p, C *c) : ptr(p), count(c) {
			++(*count);
		}

		void ref(const SmartPtr<T, C, Proxy> &p) {
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

		T *ptr;
		C *count;

};

}
}

#endif // SMARTPTR_H
