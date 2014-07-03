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

#include <utility>
#include <algorithm>
#include <initializer_list>
#include <n/Types.h>
#include <cstring>
#include "utils.h"

namespace n {
namespace core {

template<typename T>
class Array
{
	template<typename... Args>
	void construct(const T &a, const Args&... args) {
		append(a);
		construct(args...);
	}

	void construct(const T &a) {
		append(a);
	}

	public:
		typedef T* iterator;
		typedef T const * const_iterator;

		explicit Array(const uint s = 0) : data(s ? (T *)malloc(s * sizeof(T)) : 0), dataEnd(data), allocEnd(data + s) {
		}

		template<typename... Args>
		Array(const T &a, const T &b, const Args&... args) : Array(4) {
			construct(a, b, args...);
		}

		Array(std::initializer_list<T> il) : Array(1 << (log2ui(il.size() + 1))) {
			for(const T &e : il) {
				append(e);
			}
		}

		Array(const T *d, const uint s) : data(0), dataEnd(0), allocEnd(0) {
			assign((T *)d, (T *)d + s);
		}

		Array(const Array<T> &arr) : data(0), dataEnd(0), allocEnd(0) {
			assign(arr);
		}

		Array(Array<T> &&arr) : data(0), dataEnd(0), allocEnd(0) {
			swap(arr);
		}

		~Array() {
			clear();
		}

		static Array<T> filled(uint si, const T &t = T()) {
			Array<T> a(si);
			for(uint i = 0; i != si; i++) {
				a.append(t);
			}
			return a;
		}

		uint getCapacity() const {
			return allocEnd - data;
		}

		uint size() const {
			return dataEnd - data;
		}

		void setCapacity(uint cap) {
			allocate(cap);
		}

		void setMinCapacity(uint cap) {
			cap = computeSize(cap);
			if(allocEnd - data < cap) {
				setCapacity(cap);
			}
		}

		bool isEmpty() const {
			return data == dataEnd;
		}

		void swap(Array<T> &&arr) {
			swap(arr);
		}

		void swap(Array<T> &arr) {
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

		void clear() {
			allocate(0);
		}

		void squeeze() {
			setCapacity(dataEnd - data);
		}

		Array<T> &append(const T &e) {
			if(dataEnd == allocEnd) {
				allocate();
			}
			/*if(TypeInfo<T>::isPrimitive) {
				*dataEnd = e;
				dataEnd++;
			} else {*/
				new(dataEnd++) T(e);
			//}
			return *this;
		}

		Array<T> &append(T &&e) {
			if(dataEnd == allocEnd) {
				allocate();
			}
			new(dataEnd++) T(std::move(e));
			return *this;
		}

		Array<T> &append(const Array<T> &e) {
			if(e.isEmpty()) {
				return *this;
			}
			uint eSize = e.size();
			if(dataEnd + eSize > allocEnd) {
				allocate(computeSize(size() + eSize));
			}
			iterator it = e.data;
			if(TypeInfo<T>::isPrimitive) {
				memcpy(dataEnd, it, eSize * sizeof(T));
				dataEnd += eSize;
			} else {
				for(; it != e.dataEnd; it++) {
					*dataEnd = std::move(*it);
					dataEnd++;
				}
			}
			return *this;
		}

		iterator insert(iterator it, const T &e) {
			if(it == dataEnd) {
				append(e);
				return dataEnd - 1;
			}
			if(dataEnd == allocEnd) {
				uint index = it - data;
				allocate();
				it = data + index;
			}
			if(TypeInfo<T>::isPrimitive) {
				memmove(it + 1, it, (dataEnd - it) * sizeof(T));
				*it = e;
			} else {
				for(iterator i = dataEnd; i != it;) {
					new(i) T(*(i - 1));
					 i--;
					 i->~T();
				}
				new(it) T(e);
			}
			dataEnd++;
			return it;
		}

		iterator insert(const T &e) {
			append(e);
			return dataEnd - 1;
		}

		iterator insert(const uint &index, const T &e) {
			T *it = data + index;
			insert(it, e);
		}

		iterator insert(iterator beg, iterator en, iterator pos) {
			uint a = en - beg;
			uint index = pos - data;
			setMinCapacity(size() + a);
			set(data + index + a, data + index, dataEnd);
			set(data + index, beg, en);
			dataEnd += a;
			return data + index + a;
		}

		void pop() {
			dataEnd--;
			if(!TypeInfo<T>::isPrimitive) {
				dataEnd->~T();
			}
		}

		void makeEmpty() {
			if(!TypeInfo<T>::isPrimitive) {
				for(iterator i = begin(); i != end(); i++) {
					i->~T();
				}
			}
			dataEnd = data;
		}

		iterator remove(const iterator &it) {
			if(it == dataEnd - 1) {
				pop();
				return dataEnd;
			}
			if(!TypeInfo<T>::isPrimitive) {
				for(iterator i = it; i != dataEnd - 1; i++) {
					i->~T();
					new(i) T(*(i + 1));
				}
				dataEnd->~T();
			} else {
				memmove(it, it + 1, (dataEnd - it) * sizeof(T));
			}

			dataEnd--;
			return it;
		}

		iterator remove(const uint &index) {
			return remove(getIterator(index));
		}

		void assign(const Array<T> &&arr) {
			swap(arr);
		}

		void assign(const Array<T> &arr) {
			if(&arr == this) {
				return;
			}
			clear(data, dataEnd);
			dataEnd = data;
			if(arr.isEmpty()) {
				return;
			}
			setCapacity(arr.getCapacity());
			if(TypeInfo<T>::isPrimitive) {
				dataEnd = data + arr.size();
				memcpy(data, arr.data, arr.size() * sizeof(T));
			} else {
				for(const T &t : arr) {
					append(t);
				}
			}
		}

		void assign(iterator first, iterator last) {
			clear(data, dataEnd);
			uint si = last - first;
			allocate(computeSize(si));
			set(data, first, last);
		}

		void set(iterator position, iterator first, iterator last) {
			if(TypeInfo<T>::isPrimitive) {
				uint tcpy = last - first;
				memmove(position, first, tcpy * sizeof(T));
			} else {
				while(first != last) {
					position->~T();
					new(position++) T(*first);
					first++;
				}
			}
		}

		void set(iterator position, const Array<T> &arr) {
			set(position, arr.data, arr.dataEnd);
		}

		void sort() {
			std::sort(data, dataEnd);
		}

		template<typename U>
		void sort(U f) {
			std::sort(data, dataEnd, f);
		}


		template<typename U>
		iterator find(const U &f, const_iterator from) {
			for(iterator i = const_cast<iterator>(from); i != end(); i++) {
				if(f(*i)) {
					return i;
				}
			}
			return end();
		}

		template<typename U>
		const_iterator find(const U &f, const_iterator from) const {
			for(const_iterator i = from; i != end(); i++) {
				if(f(*i)) {
					return i;
				}
			}
			return end();
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
		uint count(const U &f) const {
			uint c = 0;
			for(const_iterator i = begin(); i != end(); i++) {
				if(f(*i)) {
					return c++;
				}
			}
			return c;
		}

		template<typename V>
		bool exists(const V &f) const {
			for(const_iterator i = begin(); i != end(); i++) {
				if(f(*i)) {
					return true;
				}
			}
			return false;
		}

		iterator find(const T &e) {
			return find([&](const T &t) { return t == e; }, begin());
		}

		iterator find(const T &e, const_iterator from) {
			return find([&](const T &t) { return t == e; }, from);
		}

		const_iterator find(const T &e) const {
			return find([&](const T &t) { return t == e; }, begin());
		}

		const_iterator find(const T &e, const_iterator from) const {
			return find([&](const T &t) { return t == e; }, from);
		}

		uint count(const T &e) const {
			return count([&](const T &t) { return t == e; });
		}

		bool exists(const T &e) const {
			return exists([&](const T &t) { return t == e; });
		}

		const T &first() const {
			return *data;
		}

		const T &last() const {
			return *(dataEnd - 1);
		}

		T &first() {
			return *data;
		}

		T &last() {
			return *(dataEnd - 1);
		}

		const T &get(uint index) const {
			if(index + data >= dataEnd) {
				throw nIndexOutOfBoundException(index, size());
			}
			return operator[](index);
		}

		T &get(uint index) {
			if(index + data >= dataEnd) {
				throw nIndexOutOfBoundException(index, size());
			}
			return operator[](index);
		}

		iterator getIterator(uint index) {
			return data + index;
		}

		const_iterator getConstIterator(uint index) const {
			return (const_iterator)(data + index);
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

		bool isValid(const_iterator i) const {
			return i >= data && i < dataEnd;
		}

		Array<T> operator+(const T &e) const {
			Array<T> a(*this);
			a.append(e);
			return a;
		}

		Array<T> operator+(const Array<T> &e) const {
			Array<T> a(*this);
			a.append(e);
			return a;
		}

		Array<T> &operator+=(const T &e) {
			append(e);
			return *this;
		}

		Array<T> &operator+=(const Array<T> &e) {
			append(e);
			return *this;
		}

		Array<T> &operator<<(const T &e) {
			append(e);
			return *this;
		}

		Array<T> &operator<<(const Array<T> &e) {
			append(e);
			return *this;
		}

		Array<T> &operator=(const Array<T> &arr) {
			assign(arr);
			return *this;
		}

		Array<T> &operator=(Array<T> &&arr) {
			swap(arr);
			return *this;
		}

		T &operator[](uint index) {
			return *(data + index);
		}

		const T &operator[](const uint index) const {
			return data[index];
		}

		bool operator==(const Array<T> &arr) const {
			if(size() == arr.size()) {
				for(uint i = 0, s = size(); i != s; i++) {
					if(arr[i] != data[i]) {
						return false;
					}
				}
				return true;
			}
			return false;
		}

		bool operator!=(const Array<T> &arr) const {
			return !operator==(arr);
		}

		bool operator<(const Array<T> &arr) const {
			T *a = data;
			T *b = arr.data;
			for(; a != dataEnd && b != arr.dataEnd; a++, b++) {
				if(!((*a) == (*b))) {
					if((*a) < (*b)) {
						return true;
					}
					return false;
				}
			}
			return size() < arr.size();
		}

		template<typename U>
		void foreach(const U &f) {
			std::for_each(begin(), end(), f);
		}

		template<typename U>
		void foreach(const U &f) const {
			std::for_each(begin(), end(), f);
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
		Array<T> mapped(const V &f) const {
			Array<T> a(size());
			foreach([&](const T &e) { a.append(f(e)); });
			return a;
		}

		template<typename V>
		void map(const V &f) {
			foreach([&](T &e) { e = f(e); });
		}

		template<typename U>
		Array<T> filtered(const U &f) const {
			Array<T> a(log2ui(size()));
			foreach([&](const T &e) {
				if(f(e)) {
					a.append(e);
				}
			});
			return a;
		}

		template<typename U>
		void filter(const U &f) {
			Array<T>::iterator it = begin();
			foreach([&](T &e) {
				if(f((const T &)e)) {
					*it = std::move(e);
					it++;
				}
			});
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

		Array<T> shuffled() const {
			Array<T> sh(*this);
			sh.shuffle();
			return sh;
		}

		void reverse() {
			uint s = size() / 2;
			for(uint i = 0; i != s; i++) {
				std::swap(data[i], dataEnd[-(i + 1)]);
			}
		}

		Array<T> reversed() const {
			Array<T> re(*this);
			re.reverse();
			return re;
		}


	private:
		void clear(T *from, T *to) {
			if(!TypeInfo<T>::isPrimitive) {
				for(T *it = from; it != to; it++) {
					it->~T();
				}
			}
		}

		uint computeSize(uint s) {
			if(s) {
				return 1 << (log2ui(s) + 1);
			}
			return 4;
		}

		uint computeSize() {
			return computeSize(size());
		}

		void allocate(uint s) {
			uint si = size();
			if(si > s) {
				clear(data + s, dataEnd);
				si = s;
			}
			if(s) {
				//data = (T *)realloc(data, s * sizeof(T));
				if(TypeInfo<T>::isPrimitive) {
					data = (T *)realloc(data, s * sizeof(T));
				} else {
					T *d = (T *)malloc(s * sizeof(T));
					for(uint i = 0; i != si; i++) {
						new(d + i) T(std::move(data[i]));
					}
					free(data);
					data = d;
				}
			} else {
				free(data);
				allocEnd = dataEnd = data = 0;
			}
			dataEnd = data + si;
			allocEnd = data + s;
		}

		void allocate() {
			allocate(computeSize());
		}

		T *data;
		T *dataEnd;
		T *allocEnd;
};


} //core
} //n

#endif // N_CORE_ARRAY_H
