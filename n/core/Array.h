/*******************************
Copyright (C) 2009-2010 grégoire ANGERAND

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

#ifndef N_CORE_ARRAY_H
#define N_CORE_ARRAY_H

#include <n/Types.h>
#include "utils.h"
#include <cstring>
#include <algorithm>

namespace n {
namespace core {

class DefaultArrayResizePolicy
{
	public:
		uint grow(uint size) const {
			return size < 2 ? 4 : 1 << (log2ui(size) + 1);
		}

		bool shrink() const {
			return true;
		}
};

template<typename T, typename ResizePolicy = DefaultArrayResizePolicy>
class Array : private ResizePolicy
{
	template<bool C, typename U>
	struct Adder
	{
		Adder(Array<T, ResizePolicy> &array, const U &e) {
			array.appendSimple(e);
		}
	};

	template<typename U>
	struct Adder<false, U>
	{
		Adder(Array<T, ResizePolicy> &array, const U &e) {
			array.appendCollection(e);
		}
	};

	public:
		typedef T * iterator;
		typedef T const * const_iterator;

		Array() : data(0), dataEnd(0), allocEnd(0) {
		}

		template<typename C>
		Array(const C &o) : Array(o.size()) {
			append(o);
		}

		Array(uint s) : Array() {
			setCapacityUnsafe(0, s);
		}

		Array(int s) : Array() {
			setCapacityUnsafe(0, s);
		}

		template<typename A, typename B, typename... Args>
		Array(const A &a, const B &b, const Args&... args) : Array(this->grow(sizeof...(args) + 2)) {
			append(a);
			append(b);
			append(args...);
		}

		template<typename C>
		void append(const C &c) {
			Adder<TypeConversion<C, T>::exists, C>(*this, c);
		}

		template<typename A, typename B, typename... Args>
		void append(const A &a, const B &b, const Args&... args) {
			append(a);
			append(b);
			append(args...);
		}

		template<typename A, typename... Args>
		void insert(const A &a, const Args&... args) {
			append(a);
			append(args...);
		}

		template<typename I>
		iterator insert(I b, I e, iterator position) {
			if(position == dataEnd) {
				for(; b != e; ++b) {
					append(*b);
				}
				return end();
			}
			Array<T> buffer;
			buffer.assign(begin(), position);
			buffer.insert(b, e, buffer.end());
			uint pos = buffer.size();
			buffer.insert(position, end(), buffer.end());
			swap(buffer);
			return data + pos;
		}

		template<typename C>
		void assign(const C &c) {
			makeEmpty();
			append(c);
		}

		template<typename I>
		void assign(I b, I e) {
			makeEmpty();
			for(; b != e; ++b) {
				append(*b);
			}
		}

		template<typename RP>
		void swap(Array<T, RP> &arr) {
			T *e = arr.dataEnd;
			T *a = arr.allocEnd;
			T *d = arr.data;
			arr.dataEnd = dataEnd;
			arr.allocEnd = allocEnd;
			arr.data = data;
			dataEnd = e;
			allocEnd = a;
			data = d;
		}

		void pop() {
			dataEnd--;
			dataEnd->~T();
			shrinkIfNeeded();
		}

		uint size() const {
			return dataEnd - data;
		}

		void makeEmpty() {
			clear(data, size());
			dataEnd = data;
		}

		void clear() {
			clear(data, size());
			free(data);
			data = dataEnd = allocEnd = 0;
		}

		const T &operator[](uint i) const {
			return data[i];
		}

		T &operator[](uint i) {
			return data[i];
		}

		template<typename C>
		bool operator==(const C &c) const {
			if(c.size() != size()) {
				return false;
			}
			const T *i = data;
			for(typename C::const_iterator it = c.begin(), en = c.end(); it != en; it++) {
				if(!(*it == *i)) {
					return false;
				}
				i++;
			}
			return true;
		}

		template<typename C>
		bool operator!=(const C &c) const {
			return !(*this == c);
		}

		void setCapacity(uint cap) {
			uint s = size();
			if(cap < s) {
				clear(data + cap, s - cap);
			}
			setCapacityUnsafe(s, cap);
		}

		void setMinCapacity(uint s) {
			setCapacity(this->grow(s));
		}

		uint getCapacity() const {
			return allocEnd - data;
		}

		void squeeze() {
			uint s = size();
			setCapacityUnsafe(s, s);
		}

		iterator getIterator(uint i) {
			return data + i;
		}

		const_iterator getIterator(uint i) const {
			return (const_iterator)(data + i);
		}

		const_iterator begin() const {
			return (const_iterator)data;
		}

		const_iterator end() const {
			return (const_iterator)dataEnd;
		}

		const_iterator cbegin() const {
			return (const_iterator)data;
		}

		const_iterator cend() const {
			return (const_iterator)dataEnd;
		}

		iterator begin() {
			return data;
		}

		iterator end() {
			return dataEnd;
		}

		bool isEmpty() const {
			return data == dataEnd;
		}

		T &first() {
			return *data;
		}

		T &last() {
			return *(dataEnd - 1);
		}

		const T &first() const {
			return *data;
		}

		const T &last() const {
			return *(dataEnd - 1);
		}

		bool isValid(const_iterator i) const {
			return i >= data && i < dataEnd;
		}

		bool isSorted() const {
			if(isEmpty()) {
				return true;
			}
			const_iterator l = begin();
				for(const_iterator it = begin() + 1; it != end(); it++) {
				if(*it < *l) {
					return false;
				}
				l = it;
			}
			return true;
		}

		template<typename U>
		iterator findOne(const U &f, const_iterator from) {
			for(iterator i = const_cast<iterator>(from); i != end(); i++) {
				if(f(*i)) {
					return i;
				}
			}
			return end();
		}

		template<typename U>
		const_iterator findOne(const U &f, const_iterator from) const {
			for(const_iterator i = from; i != end(); i++) {
				if(f(*i)) {
					return i;
				}
			}
			return end();
		}

		template<typename U>
		uint countAll(const U &f) const {
			uint c = 0;
			for(const_iterator i = begin(); i != end(); i++) {
				if(f(*i)) {
					c++;
				}
			}
			return c;
		}

		template<typename V>
		bool existsOne(const V &f) const {
			for(const_iterator i = begin(); i != end(); i++) {
				if(f(*i)) {
					return true;
				}
			}
			return false;
		}

		template<typename U>
		iterator find(const U &f, const_iterator from) {
			return findOne(f, from);
		}

		template<typename U>
		const_iterator find(const U &f, const_iterator from) const {
			return findOne(f, from);
		}

		template<typename U>
		uint count(const U &f) const {
			return countAll(f);
		}

		template<typename V>
		bool exists(const V &f) const {
			return existsOne(f);
		}

		iterator find(const T &e) {
			return findOne([&](const T &t) { return t == e; }, begin());
		}

		iterator find(const T &e, const_iterator from) {
			return findOne([&](const T &t) { return t == e; }, from);
		}

		const_iterator find(const T &e) const {
			return findOne([&](const T &t) { return t == e; }, begin());
		}

		const_iterator find(const T &e, const_iterator from) const {
			return findOne([&](const T &t) { return t == e; }, from);
		}

		uint count(const T &e) const {
			return countAll([&](const T &t) { return t == e; });
		}

		bool exists(const T &e) const {
			return existsOne([&](const T &t) { return t == e; });
		}

		template<typename U>
		void foreach(const U &f) {
			std::for_each(begin(), end(), f);
		}

		template<typename U>
		void foreach(const U &f) const {
			std::for_each(begin(), end(), f);
		}

		template<typename V, typename C = Array<typename std::result_of<V(const T &)>::type, ResizePolicy>>
		C mapped(const V &f) const {
			C a;
			foreach([&](const T &e) { a.insert(f(e)); });
			return a;
		}

		template<typename U, typename C = Array<T, ResizePolicy>>
		C filtered(const U &f) const {
			C a;
			foreach([&](const T &e) {
				if(f(e)) {
					a.insert(e);
				}
			});
			return a;
		}

		template<typename C = Array<T, ResizePolicy>>
		C shuffled() const {
			C sh(*this);
			sh.shuffle();
			return sh;
		}

		template<typename C = Array<T, ResizePolicy>>
		C reversed() const {
			C re(*this);
			re.reverse();
			return re;
		}

		template<typename U>
		bool forall(const U &f) const {
			for(const T &t : *this) {
				if(!f(t)) {
					return false;
				}
			}
			return true;
		}

		template<typename V>
		void map(const V &f) {
			foreach([&](T &e) { e = f(e); });
		}

		template<typename U>
		void filter(const U &f) {
			Array<T, ResizePolicy>::iterator it = begin();
			foreach([&](T &e) {
				if(f((const T &)e)) {
					*it = std::move(e);
					it++;
				}
			});
			dataEnd = it;
			setCapacity(it - begin());
		}

		template<typename V, typename U>
		V foldRight(const U &f, V def = V(0)) {
			V acc(def);
			foreach([&](const T &t) { acc = f(t, acc); });
			return acc;
		}

		void shuffle() {
			uint s = size();
			for(uint i = 0; i != s; i++) {
				uint a = random(s);
				if(a != i) {
					std::swap(data[a], data[i]);
				}
			}
		}

		void reverse() {
			uint s = size() / 2;
			for(uint i = 0; i != s; i++) {
				std::swap(data[i], dataEnd[-(i + 1)]);
			}
		}


	private:
		void copy(T *dst, const T *src, uint n) {
			if(TypeInfo<T>::isPrimitive) {
				memcpy(dst, src, sizeof(T) * n);
			} else {
				for(; n; n--) {
					new(dst++) T(*(src++));
				}
			}
		}

		void move(T *dst, const T *src, uint n) {
			if(TypeInfo<T>::isPrimitive) {
				memmove(dst, src, sizeof(T) * n);
			} else {
				for(; n; n--) {
					new(dst++) T(std::move(*(src++)));
				}
			}
		}

		void clear(T *src, uint n) {
			if(!TypeInfo<T>::isPrimitive) {
				for(; n; n--) {
					(src++)->~T();
				}
			}
		}

		void expend() {
			uint s = size();
			uint ns = this->grow(s);
			setCapacityUnsafe(s, ns);
		}

		void shrinkIfNeeded() {
			if(this->shrink()) {
				uint cc = getCapacity();
				uint s = size();
				uint tc = this->grow(s);
				if(cc != tc) {
					setCapacityUnsafe(s, tc);
				}
			}
		}

		void appendSimple(const T &e) {
			if(dataEnd == allocEnd) {
				expend();
			}
			new(dataEnd++) T(e);
		}

		template<typename C>
		void appendCollection(const C &c) {
			setMinCapacity(size() + c.size());
			for(const auto &e : c) {
				append(e);
			}
		}

		void append() {
		}

		void setCapacityUnsafe(uint s, uint ns) {
			if(TypeInfo<T>::isPrimitive) {
				data = (T *)realloc(data, ns * sizeof(T));
			} else {
				T *n = (T *)malloc(ns * sizeof(T));
				move(n, data, s);
				clear(data, s);
				free(data);
				data = n;
			}
			dataEnd = data + s;
			allocEnd = data + ns;
		}

		T *data;
		T *dataEnd;
		T *allocEnd;
};


} //core
} //n

#endif // N_CORE_ARRAY_H
