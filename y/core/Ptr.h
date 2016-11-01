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
class Ptr : NonCopyable {
	public:
		explicit Ptr(NotOwned<T*>&& p = nullptr) : _ptr(p) {
		}

		explicit Ptr(T&& p) : Ptr(new T(std::move(p))) {
		}

		Ptr(nullptr_t p) : _ptr(p) {
		}

		Ptr(Ptr&& p) : _ptr(nullptr) {
			swap(p);
		}

		~Ptr() {
			delete _ptr;
		}

		Ptr& operator=(Ptr&& p) {
			swap(p);
			return *this;
		}

		Ptr& operator=(NotOwned<T*>&& p) {
			Ptr ptr(std::move(p));
			swap(ptr);
			return *this;
		}

		Ptr& operator=(nullptr_t p) {
			Ptr ptr(p);
			swap(ptr);
			return *this;
		}

		void swap(Ptr& p) {
			std::swap(_ptr, p._ptr);
		}

		const T& operator*() const {
			return *_ptr;
		}

		T& operator*() {
			return *_ptr;
		}

		Owned<T const*> operator->() const {
			return _ptr;
		}

		Owned<T*> operator->() {
			return _ptr;
		}

		operator Owned<void*>() {
			return _ptr;
		}

		operator Owned<void const*>() const {
			return _ptr;
		}

		bool operator<(const Owned<T* const> t) const {
			return _ptr < t;
		}

		bool operator>(const Owned<T* const> t) const {
			return _ptr > t;
		}

		bool operator!() const {
			return is_null();
		}

		bool is_null() const {
			return !_ptr;
		}

		Owned<T*> as_ptr() {
			return _ptr;
		}

		Owned<T const*> as_ptr() const {
			return _ptr;
		}

	protected:
		T* _ptr;
};

template<typename T, typename C = u32>
class Rc : public Ptr<T> {
	using Ptr<T>::_ptr;
	public:
		Rc() : Ptr<T>(nullptr), _count(nullptr) {
		}

		explicit Rc(NotOwned<T*>&& p) : Ptr<T>(std::move(p)), _count(new C(1)) {
		}

		explicit Rc(T&& p) : Rc(new T(std::move(p))) {
		}

		Rc(const Rc& p) : Rc() {
			ref(p);
		}

		Rc(Rc&& p) : Rc() {
			swap(p);
		}

		Rc(Ptr<T>&& p) : Rc(nullptr) {
			Ptr<T>::swap(p);
		}

		~Rc() {
			unref();
		}

		void swap(Rc& p) {
			std::swap(_ptr, p._ptr);
			std::swap(_count, p._count);
		}

		C ref_count() const {
			return *_count;
		}

		Rc& operator=(const Rc& p) {
			if(p._count != _count) {
				unref();
				ref(p);
			}
			return *this;
		}

		Rc& operator=(nullptr_t) {
			unref();
			return *this;
		}

		Rc& operator=(Rc&& p) {
			swap(p);
			return *this;
		}

	private:
		friend class Rc<const T>;
		friend class Rc<typename std::remove_const<T>::type>;

		// only called by other Rc's so we take ownedship as well
		Rc(T* p, C* c) : Ptr<T>(p), _count(c) {
			++(*_count);
		}

		void ref(const Rc& p) {
			if((_count = p._count)) {
				(*_count)++;
			}
			_ptr = p._ptr;
		}

		void unref() {
			if(_count && !--(*_count)) {
				delete _ptr;
				delete _count;
			}
			_ptr = nullptr;
			_count = nullptr;
		}

		C* _count;
};

template<typename T>
inline auto ptr(T&& t) {
	return Ptr<typename std::remove_reference<T>::type>(std::forward<T>(t));
}

template<typename T>
inline auto rc(T&& t) {
	return Rc<typename std::remove_reference<T>::type>(std::forward<T>(t));
}

}
}


#endif // Y_CORE_PTR_H

