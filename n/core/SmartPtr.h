/*******************************
Copyright (C) 2013-2014 gr�goire ANGERAND

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

class NoLock
{
	public:
		void swap(const NoLock &) {}
		void ref(const NoLock &) {}
		void unref(bool) {}
		void lock() {}
		void unlock() {}
};

template<typename T, typename Proxy = NoProxy<T>, typename LockingPolicy = NoLock>
class SmartPtr : private LockingPolicy
{
	public:
		SmartPtr(T *&&p = 0) : LockingPolicy(), ptr(p), count(ptr ? new uint(1) : 0) {
		}

		SmartPtr(const SmartPtr<T, Proxy, LockingPolicy> &p) : LockingPolicy(), ptr(0), count(0) {
			ref(p);
		}

		~SmartPtr() {
			unref();
		}

		LockingPolicy &getLockingPolicy() {
			return *((LockingPolicy *)this);
		}

		void swap(SmartPtr<T, Proxy, LockingPolicy> &p) {
			p.lock();
			this->lock();
			T *pt = ptr;
			uint *c = count;
			count = p.count;
			ptr = p.ptr;
			p.ptr = pt;
			p.count = c;
			LockingPolicy::swap(p);
			this->unlock();
			p.unlock();
		}

		bool isNull() const {
			return !ptr;
		}

		uint getReferenceCount() const {
			return *count;
		}

		T &operator*() const {
			return *ptr;
		}

		Proxy operator->() const {
			return Proxy(ptr);
		}

		SmartPtr<T, Proxy, LockingPolicy> &operator=(const SmartPtr<T, Proxy, LockingPolicy> &p) {
			unref();
			ref(p);
			return *this;
		}

		SmartPtr<T, Proxy, LockingPolicy> &operator=(SmartPtr<T, Proxy, LockingPolicy> &&p) {
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

		operator SmartPtr<const T>() const {
			return *this;
		}

	private:
		void ref(const SmartPtr<T, Proxy, LockingPolicy> &p) {
			LockingPolicy::ref(p);
			this->lock();
			ptr = p.ptr;
			if((count = p.count)) {
				(*count)++;
			}
			this->unlock();
		}

		void unref() {
			bool d = false;
			this->lock();
			if(count && !--(*count)) {
				delete ptr;
				delete count;
				d = true;
			}
			ptr = 0;
			count = 0;
			this->unlock();
			LockingPolicy::unref(d);
		}

		T *ptr;
		uint *count;

};

}
}

#endif // SMARTPTR_H
