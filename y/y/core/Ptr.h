/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#ifndef Y_CORE_RC_H
#define Y_CORE_RC_H

#include <y/utils.h>

namespace y {
namespace core {

namespace detail {

template<typename T>
class Ptr : NonCopyable {
	public:
		using value_type = std::remove_reference_t<T>;
		using pointer = std::remove_extent_t<T>*;
		using const_pointer = const std::remove_extent_t<T>*;

		T& operator*() const {
			return *_ptr;
		}

		pointer operator->() const {
			return _ptr;
		}

		operator std::conditional_t<std::is_const_v<T>, const void*, void*>() const {
			return _ptr;
		}

		bool operator<(const_pointer t) const {
			return _ptr < t;
		}

		bool operator>(const_pointer t) const {
			return _ptr > t;
		}

		bool operator!() const {
			return is_null();
		}

		bool is_null() const {
			return !_ptr;
		}

		pointer as_ptr() const {
			return _ptr;
		}

		template<typename = void>
		auto& operator[](usize i) const {
			static_assert(std::is_array_v<T>, "T is not an array");
			return _ptr[i];
		}

	protected:
		template<typename Y>
		static constexpr bool can_downcast() {
			return std::is_same_v<Y, T> || std::is_polymorphic_v<Y> || std::is_pointer_v<Y>;
		}

		Ptr() = default;

		Ptr(pointer&& p) : _ptr(p) {
		}

		Owner<pointer> _ptr = nullptr;

		void destroy() {
			if constexpr(std::is_array_v<T>) {
				delete[] _ptr;
			} else {
				delete _ptr;
			}
		}
};

}

template<typename T>
class Unique : public detail::Ptr<T> {

	using Base = detail::Ptr<T>;
	using Base::_ptr;

	public:
		using pointer = typename Base::pointer;
		using const_pointer = typename Base::const_pointer;

		Unique() = default;

		Unique(pointer&& p) : Base(std::move(p)) {
		}

		Unique(std::remove_const_t<T>&& p) : Unique(new T(std::move(p))) {
		}

		template<typename Y>
		Unique(Unique<Y>&& p) {
			swap(p);
		}


		~Unique() {
			Base::destroy();
		}

		Unique& operator=(Unique&& p) {
			swap(p);
			return *this;
		}

		Unique& operator=(pointer&& p) {
			Unique ptr(std::move(p));
			swap(ptr);
			return *this;
		}

	private:
		template<typename Y>
		void swap(Unique<Y>& p) {
			static_assert(Base::template can_downcast<Y>(), "Dangerous slicing");
			std::swap(_ptr, p._ptr);
		}

};

template<typename T, typename C = u32>
class Rc : public detail::Ptr<T> {

	using Base = detail::Ptr<T>;
	using Base::_ptr;

	public:
		using pointer = typename Base::pointer;
		using const_pointer = typename Base::const_pointer;

		Rc() = default;

		Rc(pointer&& p) : Base(std::move(p)), _count(this->is_null() ? nullptr : new C(1)) {
		}

		Rc(const Rc& p) : Rc(p._ptr, p._count) {
		}

		Rc(Rc&& p) {
			swap(p);
		}

		Rc(std::remove_const_t<T>&& t) : Rc(new T(std::move(t))) {
		}

		template<typename Y>
		Rc(const Rc<Y, C>& p) : Rc(p._ptr, p._count) {
			static_assert(Base::template can_downcast<Y>(), "Dangerous slicing");
		}

		~Rc() {
			unref();
		}

		void swap(Rc& p) {
			std::swap(_ptr, p._ptr);
			std::swap(_count, p._count);
		}

		const C& ref_count() const {
			return *_count;
		}

		Rc& operator=(const Rc& p) {
			if(p._count != _count) {
				unref();
				ref(p);
			}
			return *this;
		}

		Rc& operator=(Rc&& p) {
			swap(p);
			return *this;
		}

		template<typename Y>
		Rc& operator=(Y*&& p) {
			return operator=(Rc(p));
		}

	private:
		template<typename U, typename D>
		friend class Rc;

		C& mut_ref_count() {
			return *_count;
		}

		Rc(T* p, C* c) : Base(std::move(p)), _count(c) {
			if(_count) {
				++mut_ref_count();
			}
		}

		void ref(const Rc& p) {
			if(p._count && (_count = p._count)) {
				++mut_ref_count();
			}
			_ptr = p._ptr;
		}

		void unref() {
			if(_count && !--mut_ref_count()) {
				Base::destroy();
				delete _count;
			}
			_ptr = nullptr;
			_count = nullptr;
		}


		C* _count = nullptr;
};


template<typename T>
Unique(T*&&) -> Unique<T>;

template<typename T>
Rc(T*&&) -> Rc<T>;

}
}

namespace std {
template<typename T>
struct hash<y::core::Unique<T>> : hash<T*> {
	auto operator()(const y::core::Unique<T>& p) const {
		return hash<T*>::operator()(p.as_ptr());
	}
};
template<typename T, typename C>
struct hash<y::core::Rc<T, C>> : hash<T*> {
	auto operator()(const y::core::Rc<T, C>& p) const {
		return hash<T*>::operator()(p.as_ptr());
	}
};
}


#endif // Y_CORE_RC_H

