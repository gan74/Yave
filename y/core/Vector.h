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
#ifndef Y_CORE_VECTOR
#define Y_CORE_VECTOR

#include <y/utils.h>
#include "Range.h"
#include <cstring>

namespace y {
namespace core {

struct DefaultVectorResizePolicy {
	static constexpr usize threshold = 4096;
	static constexpr usize step = 2048;

	usize ideal_capacity(usize size) const {
		if(!size) {
			return 0;
		}
		if(size < threshold) {
			return 1 << (log2ui(size) + 1);
		}
		usize steps = (size - threshold) / step;
		return threshold + (steps + 1) * step;
	}

	bool shrink(usize size, usize capacity) const {
		return !size || (capacity - size) > 2 * step;
	}
};


template<typename Elem, typename ResizePolicy = DefaultVectorResizePolicy>
class Vector : ResizePolicy {

	using Data = typename std::remove_const<Elem>::type;

	static_assert(!std::is_polymorphic<Elem>::value, "Vector<T> should not contain polymorphic objects directly");

	public:
		using iterator = Elem*;
		using const_iterator = Elem const*;
		using Element = Elem;

		Vector() : _data(nullptr), _data_end(nullptr), _alloc_end(nullptr) {
		}

		Vector(const Vector& other) : Vector() {
			set_min_capacity(other.size());
			for(const Elem& e : other) {
				append(e);
			}
		}

		Vector(Vector&& other) : Vector() {
			swap(other);
		}

		template<typename T>
		explicit Vector(T&& other) : Vector() {
			//set_min_capacity(other.size());
			append(range(std::forward<T>(other)));
		}

		Vector(std::initializer_list<Elem> l) : Vector() {
			set_min_capacity(l.size());
			for(const auto& e : l) {
				append(e);
			}
		}

		Vector(usize size, const Elem& elem) : Vector() {
			set_min_capacity(size);
			for(usize i = 0; i != size; i++) {
				append(elem);
			}
		}

		Vector& operator=(Vector&& other) {
			swap(other);
			return *this;
		}

		Vector& operator=(const Vector& other) {
			make_empty();
			set_min_capacity(other.size());
			append(range(other));
			return *this;
		}

		template<typename T>
		Vector& operator=(const T& other) {
			make_empty();
			set_min_capacity(other.size());
			append(range(other));
			return *this;
		}

		void swap(Vector& v) {
			std::swap(_data, v._data);
			std::swap(_data_end, v._data_end);
			std::swap(_alloc_end, v._alloc_end);
		}

		~Vector() {
			clear();
		}

		void append(const Element& elem) {
			if(_data_end == _alloc_end) {
				expend();
			}
			new(_data_end++) Data(elem);
		}

		void append(Element&& elem) {
			if(_data_end == _alloc_end) {
				expend();
			}
			new(_data_end++) Data(std::move(elem));
		}

		template<typename I, typename Enable = typename std::enable_if<std::is_same<typename Range<I>::Element, Element>::value>::type>
		void append(const Range<I>& rng) {
			for(const auto& i : rng) {
				append(i);
			}
		}

		Element pop() {
			--_data_end;
			Data r = std::move(*_data_end);
			_data_end->~Data();
			shrink();
			return r;
		}

		usize size() const {
			return _data_end - _data;
		}

		bool is_empty() const {
			return _data == _data_end;
		}

		usize capacity() const {
			return _alloc_end - _data;
		}

		const_iterator begin() const {
			return _data;
		}

		const_iterator end() const {
			return _data_end;
		}

		const_iterator cbegin() const {
			return _data;
		}

		const_iterator cend() const {
			return _data_end;
		}

		iterator begin() {
			return _data;
		}

		iterator end() {
			return _data_end;
		}

		const Element& operator[](usize i) const {
			return _data[i];
		}

		Element& operator[](usize i) {
			return _data[i];
		}

		const Element& first() const {
			return *_data;
		}

		Element& first() {
			return *_data;
		}

		const Element& last() const {
			return *(_data_end - 1);
		}

		Element& last() {
			return *(_data_end - 1);
		}

		Range<iterator> to_range() {
			return core::range(*this);
		}

		Range<const_iterator> to_range() const {
			return core::range(*this);
		}

		void set_capacity(usize cap) {
			unsafe_set_capacity(size(), cap);
		}

		void set_min_capacity(usize min_cap) {
			unsafe_set_capacity(size(), this->ideal_capacity(min_cap));
		}

		void clear() {
			unsafe_set_capacity(0, 0);
		}

		void squeeze() {
			set_capacity(size());
		}

		void make_empty() {
			clear(_data, _data_end);
			_data_end = _data;
		}

		bool operator==(const Vector& v) const {
			usize s = size();
			if(s == v.size()) {
				for(usize i = 0; i != s; i++) {
					if(_data[i] != v[i]) {
						return false;
					}
				}
				return true;
			}
			return false;
		}

		bool operator!=(const Vector& v) const {
			return !operator==(v);
		}


	private:
		void move_range(Data* dst, Data* src, usize n) {
			if(std::is_pod<Data>::value) {
				memmove(dst, src, sizeof(Data) * n);
			} else {
				for(; n; n--) {
					new(dst++) Data(std::move(*(src++)));
				}
			}
		}

		void clear(Data* beg, Data* en) {
			if(!std::is_pod<Data>::value) {
				while(en != beg) {
					(--en)->~Data();
				}
			}
		}

		void expend() {
			unsafe_set_capacity(size(), this->ideal_capacity(size() + 1));
		}

		void shrink() {
			usize current = size();
			usize cap = capacity();
			if(current < cap && ResizePolicy::shrink(current, cap)) {
				unsafe_set_capacity(current, ResizePolicy::ideal_capacity(current));
			}
		}

		// uses data_end !!
		void unsafe_set_capacity(usize num_to_move, usize new_cap) {
			num_to_move = num_to_move < new_cap ? num_to_move : new_cap;

			Data* new_data = new_cap ? reinterpret_cast<Data*>(new u8[new_cap * sizeof(Data)]) : nullptr;

			move_range(new_data, _data, num_to_move);
			clear(_data, _data_end);

			delete[] reinterpret_cast<u8*>(_data);
			_data = new_data;
			_data_end = _data + num_to_move;
			_alloc_end = _data + new_cap;
		}

		Data* _data;
		Data* _data_end;
		Data* _alloc_end;
};



namespace detail {
template<typename T>
inline void append(Vector<T>&) {
}

template<typename T, typename U, typename... Args>
inline void append(Vector<T>& vec, U&& u, Args&&... args) {
	vec.append(std::forward<U>(u));
	append(vec, std::forward<Args>(args)...);
}

template<typename T, typename U>
inline void append(Vector<T>& vec, U&& u) {
	vec.append(std::forward<U>(u));
}
}

template<typename T>
inline auto vector_with_capacity(usize cap) {
	auto vec = Vector<T>();
	vec.set_min_capacity(cap);
	return vec;
}

template<typename T, typename... Args>
inline auto vector(Args... args) {
	auto vec = vector_with_capacity<T>(sizeof...(args));
	detail::append(vec, std::forward<Args>(args)...);
	return vec;
}

template<typename T, typename... Args>
inline auto vector(T&& t, Args&&... args) {
	return vector<typename std::common_type<T, Args...>::type>(std::forward<T>(t), std::forward<Args>(args)...);
}

template<typename T>
inline auto vector(std::initializer_list<T> lst) {
	return Vector<T>(lst);
}




static_assert(std::is_same<decltype(vector(1, 2, 3))::Element, int>::value, "Invalid vector(...) return type");
static_assert(std::is_same<decltype(vector(1, 2.0, 3))::Element, double>::value, "Invalid vector(...) return type");




template<typename U, typename T>
inline Vector<U>& operator<<(Vector<U>& vec, T&& t) {
	vec.append(std::forward<T>(t));
	return vec;
}

template<typename U, typename T>
inline Vector<U>& operator+=(Vector<U>& vec, T&& t) {
	vec.append(std::forward<T>(t));
	return vec;
}

template<typename U, typename T>
inline Vector<U> operator+(Vector<U> vec, T&& t) {
	vec.append(std::forward<T>(t));
	return vec;
}

}
}



#endif
