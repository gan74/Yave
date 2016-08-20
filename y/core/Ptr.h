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

#ifndef Y_CORE_PTR_H
#define Y_CORE_PTR_H

#include <y/utils.h>

namespace y {
namespace core {

template<typename T>
class Box : NonCopyable
{
	public:
		Box(T *&&p = 0) : ptr(p) {
		}

		~Box() {
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

		operator void *() {
			return ptr;
		}

		operator void const *() const {
			return ptr;
		}

		bool operator<(const T * const t) const {
			return ptr < t;
		}

		bool operator>(const T * const t) const {
			return ptr > t;
		}

		bool operator!() const {
			return is_null();
		}

		bool is_null() const {
			return !ptr;
		}

		T *as_ptr() {
			return ptr;
		}

		T const *as_ptr() const {
			return ptr;
		}

	protected:
		T *ptr;
};

template<typename T, typename C = u32>
class Rc : public Ptr<T>
{
	using Ptr<T>::ptr;
	public:
		Rc(T *&&p = 0) : Ptr<T>(std::move(p)), count(new C(1)) {
		}

		Rc(const Rc<T, C> &p) : Ptr<T>(0), count(0) {
			ref(p);
		}

		~Rc() {
			unref();
		}

		void swap(Rc<T, C> &&p) {
			std::swap(ptr, p.ptr);
			std::swap(count, p.count);
		}

		C get_ref() const {
			return count ? *count : 0;
		}

		Rc<T, C> &operator=(const SmartPtr<T, C> &p) {
			if(&p != this) {
				unref();
				ref(p);
			}
			return *this;
		}

		Rc<T, C> &operator=(Rc<T, C> &&p) {
			swap(std::move(p));
			return *this;
		}

	private:
		friend class Rc<const T>;
		friend class Rc<typename std::remove_const<T>::type>;

		Rc(T *p, C *c) : Ptr<T>(p), count(c) {
			++(*count);
		}

		void ref(const Rc<T, C> &p) {
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


#endif // Y_CORE_PTR_H

