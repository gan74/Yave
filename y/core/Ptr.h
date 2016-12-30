/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

This code is licensed under the MIT License (MIT).

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

#ifndef Y_CORE_PTR_H
#define Y_CORE_PTR_H

#include <y/utils.h>

namespace y {
namespace core {

template<typename T>
class Ptr : NonCopyable {
	public:
		explicit Ptr(Owner<T*>&& p = nullptr) : _ptr(p) {
		}

		explicit Ptr(T&& p) : Ptr(new T(std::move(p))) {
		}

		Ptr(std::nullptr_t p) : _ptr(p) {
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

		Ptr& operator=(Owner<T*>&& p) {
			Ptr ptr(std::move(p));
			swap(ptr);
			return *this;
		}

		Ptr& operator=(std::nullptr_t p) {
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

		T const* operator->() const {
			return _ptr;
		}

		T* operator->() {
			return _ptr;
		}

		operator void*() {
			return _ptr;
		}

		operator void const*() const {
			return _ptr;
		}

		bool operator<(const T* const t) const {
			return _ptr < t;
		}

		bool operator>(const T* const t) const {
			return _ptr > t;
		}

		bool operator!() const {
			return is_null();
		}

		bool is_null() const {
			return !_ptr;
		}

		T* as_ptr() {
			return _ptr;
		}

		T const* as_ptr() const {
			return _ptr;
		}

	protected:
		Owner<T*> _ptr;
};

template<typename T, typename C = u32>
class Rc : public Ptr<T> {
	using Ptr<T>::_ptr;
	public:
		Rc() : Ptr<T>(nullptr), _count(nullptr) {
		}

		Rc(std::nullptr_t) : Rc() {
		}

		explicit Rc(Owner<T*>&& p) : Ptr<T>(std::move(p)), _count(new C(1)) {
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

		Rc& operator=(std::nullptr_t) {
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

