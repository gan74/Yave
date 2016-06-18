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

namespace n {
namespace core {

template<typename T, typename RP>
Array<T,RP>::Array() : RP(), data(0), dataEnd(0), allocEnd(0) {
}

template<typename T, typename RP>
Array<T, RP>::Array(const Array<T, RP> &a) : Array<T, RP>(a.size()) {
	copy(data, a.data, a.size());
	dataEnd = data + a.size();
}

template<typename T, typename RP>
Array<T, RP>::Array(Array<T, RP> &&a) : Array<T, RP>() {
	swap(std::move(a));
}

template<typename T, typename RP>
template<typename C, typename R>
Array<T, RP>::Array(const Array<C, R> &a) : Array<T, RP>(a.size()) {
	append(a);
}

template<typename T, typename RP>
template<typename C, typename CC>
Array<T, RP>::Array(const C &c) : Array<T, RP>(c.size()) {
	assign(c.begin(), c.end());
}

template<typename T, typename RP>
template<typename C>
Array<T, RP>::Array(std::initializer_list<C> l) : Array<T, RP>((l.size())) {
	for(auto x : l) {
		append(x);
	}
}

template<typename T, typename RP>
Array<T,RP>::Array(uint s) : Array() {
	setCapacityUnsafe(0, s);
}

template<typename T, typename RP>
Array<T,RP>::Array(uint s, const T &e) : Array(s) {
	for(uint i = 0; i != s; i++) {
		append(e);
	}
}

template<typename T, typename RP>
Array<T, RP>::~Array() {
	makeEmpty();
	free(data);
}

template<typename T, typename RP>
template<typename C>
Array<T, RP> &Array<T, RP>::append(const C &c) {
	appendDispatch(c, BoolToType<!ShouldInsertAsCollection<C, T>::value>()); // <------------------------ This is the line you are looking for
	return *this;
}

template<typename T, typename RP>
template<typename C>
Array<T, RP> &Array<T, RP>::append(std::initializer_list<C> l) {
	for(auto x : l) {
		append(x);
	}
	return *this;
}

template<typename T, typename RP>
typename Array<T, RP>::iterator Array<T, RP>::remove(const T &e) {
	return erase(find(e));
}

template<typename T, typename RP>
typename Array<T, RP>::iterator Array<T, RP>::erase(const_iterator b) {
	return erase(b, b + 1);
}

template<typename T, typename RP>
typename Array<T, RP>::iterator Array<T, RP>::erase(const_iterator b, const_iterator e) {
	if(e < b) {
		std::swap(b, e);
	}
	if(b >= dataEnd) {
		return dataEnd;
	}
	uint diff = e - b;
	if(e == dataEnd) {
		clear(const_cast<TT *>(b), diff);
		dataEnd -= diff;
		shrinkIfNeeded();
		return dataEnd;
	}
	moveBack(const_cast<TT *>(b), dataEnd - b, diff);
	dataEnd -= diff;
	shrinkIfNeeded();
	return const_cast<TT *>(b);
}

template<typename T, typename RP>
template<typename I>
typename Array<T, RP>::iterator Array<T, RP>::insert(I b, I e, iterator position) {
	if(position == dataEnd) {
		for(; b != e; ++b) {
			append(*b);
		}
		return end();
	}
	Array<T, RP> buffer(size());
	buffer.assign(begin(), position);
	buffer.insert(b, e, buffer.end());
	uint pos = buffer.size();
	buffer.insert(position, end(), buffer.end());
	swap(buffer);
	return data + pos;
}

template<typename T, typename RP>
template<typename C>
typename Array<T, RP>::iterator Array<T, RP>::insert(const C &e, iterator position) {
	return insertDispatch(e, position, BoolToType<ShouldInsertAsCollection<C, T>::value>());
}

template<typename T, typename RP>
template<typename C>
typename Array<T, RP>::iterator Array<T, RP>::insert(const C &e) {
	append(e);
	return end() - 1;
}

template<typename T, typename RP>
template<typename C>
void Array<T, RP>::assign(const C &c) {
	makeEmpty();
	append(c);
}

template<typename T, typename RP>
template<typename I>
void Array<T, RP>::assign(I b, I e) {
	makeEmpty();
	for(; b != e; ++b) {
		append(*b);
	}
	shrinkIfNeeded();
}

template<typename T, typename RP>
template<typename R>
void Array<T, RP>::swap(Array<T, R> &arr) {
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

template<typename T, typename RP>
template<typename R>
void Array<T, RP>::swap(Array<T, R> &&arr) {
	swap(arr);
}

template<typename T, typename RP>
void Array<T, RP>::pop() {
	dataEnd--;
	dataEnd->~T();
	shrinkIfNeeded();
}

template<typename T, typename RP>
uint Array<T, RP>::size() const {
	return dataEnd - data;
}

template<typename T, typename RP>
uint Array<T, RP>::getCapacity() const {
	return allocEnd - data;
}

template<typename T, typename RP>
bool Array<T, RP>::isEmpty() const {
	return data == dataEnd;
}

template<typename T, typename RP>
void Array<T, RP>::makeEmpty() {
	clear(data, size());
	dataEnd = data;
}

template<typename T, typename RP>
void Array<T, RP>::clear() {
	clear(data, size());
	free(data);
	data = dataEnd = allocEnd = 0;
}

template<typename T, typename RP>
const T &Array<T, RP>::operator[](uint i) const {
	return data[i];
}

template<typename T, typename RP>
T &Array<T, RP>::operator[](uint i) {
	return data[i];
}

template<typename T, typename RP>
template<typename C>
bool Array<T, RP>::operator==(const C &l) const {
	if(size() == l.size()) {
		const_iterator a = begin();
		auto b = l.begin();
		while(a != end()) {
			if(*a++ != *b++) {
				return false;
			}
		}
		return true;
	}
	return false;
}

template<typename T, typename RP>
template<typename C>
bool Array<T, RP>::operator!=(const C &c) const {
	return !(*this == c);
}

template<typename T, typename RP>
template<typename C>
Array<T, RP> &Array<T, RP>::operator+=(const C &e) {
	append(e);
	return *this;
}

template<typename T, typename RP>
template<typename C>
Array<T, RP> &Array<T, RP>::operator<<(const C &e) {
	append(e);
	return *this;
}

template<typename T, typename RP>
template<typename C>
Array<T, RP> Array<T, RP>::operator+(const C &e) {
	Array<T, RP> x(*this);
	x.append(e);
	return x;
}

template<typename T, typename RP>
template<typename C>
Array<T, RP> &Array<T, RP>::operator=(const C &c) {
	assign(c);
	return *this;
}

template<typename T, typename RP>
Array<T, RP> &Array<T, RP>::operator=(const Array<T, RP> &e) {
	if(&e != this) {
		assign(e);
	}
	return *this;
}

template<typename T, typename RP>
Array<T, RP> &Array<T, RP>::operator=(Array<T, RP> &&arr) {
	swap(arr);
	return *this;
}

template<typename T, typename RP>
void Array<T, RP>::setCapacity(uint cap) {
	uint s = size();
	if(cap < s) {
		clear(data + cap, s - cap);
	}
	setCapacityUnsafe(s, cap);
}

template<typename T, typename RP>
void Array<T, RP>::setMinCapacity(uint s) {
	setCapacity(RP::size(s));
}

template<typename T, typename RP>
void Array<T, RP>::reserve(uint s) {
	setMinCapacity(s + size());
}

template<typename T, typename RP>
void Array<T, RP>::squeeze() {
	uint s = size();
	setCapacityUnsafe(s, s);
}

template<typename T, typename RP>
typename Array<T, RP>::iterator Array<T, RP>::getIterator(uint i) {
	return data + i;
}

template<typename T, typename RP>
typename Array<T, RP>::const_iterator Array<T, RP>::getIterator(uint i) const {
	return const_iterator(data + i);
}

template<typename T, typename RP>
typename Array<T, RP>::const_iterator Array<T, RP>::begin() const {
	return const_iterator(data);
}

template<typename T, typename RP>
typename Array<T, RP>::const_iterator Array<T, RP>::end() const {
	return const_iterator(dataEnd);
}

template<typename T, typename RP>
typename Array<T, RP>::const_iterator Array<T, RP>::cbegin() const {
	return const_iterator(data);
}

template<typename T, typename RP>
typename Array<T, RP>::const_iterator Array<T, RP>::cend() const {
	return const_iterator(dataEnd);
}

template<typename T, typename RP>
typename Array<T, RP>::iterator Array<T, RP>::begin() {
	return data;
}

template<typename T, typename RP>
typename Array<T, RP>::iterator Array<T, RP>::end() {
	return dataEnd;
}

template<typename T, typename RP>
T &Array<T, RP>::first() {
	return *data;
}

template<typename T, typename RP>
T &Array<T, RP>::last() {
	return *(dataEnd - 1);
}

template<typename T, typename RP>
const T &Array<T, RP>::first() const {
	return *data;
}

template<typename T, typename RP>
const T &Array<T, RP>::last() const {
	return *(dataEnd - 1);
}

template<typename T, typename RP>
bool Array<T, RP>::isValid(const_iterator i) const {
	return i >= data && i < dataEnd;
}

template<typename T, typename RP>
template<typename C>
void Array<T, RP>::sort(const C &c) {
	std::sort(begin(), end(), c);
}

template<typename T, typename RP>
bool Array<T, RP>::isSorted() const {
	const_iterator l = begin();
		for(const_iterator it = begin() + 1; it != end(); it++) {
		if(*it < *l) {
			return false;
		}
		l = it;
	}
	return true;
}

template<typename T, typename RP>
template<typename U>
void Array<T, RP>::foreach(const U &f) {
	std::for_each(begin(), end(), f);
}

template<typename T, typename RP>
template<typename U>
void Array<T, RP>::foreach(const U &f) const {
	std::for_each(begin(), end(), f);
}

template<typename T, typename RP>
template<typename V, typename C>
C Array<T, RP>::mapped(const V &f) const {
	C a;
	using ArrayType = Array<typename std::result_of<V(const T &)>::type, RP>;
	if(std::is_same<C, ArrayType>::value) {
		reinterpret_cast<ArrayType *>(&a)->setMinCapacity(size());
	}
	foreach([&](const T &e) { a.insert(f(e)); });
	return a;
}

template<typename T, typename RP>
template<typename V>
void Array<T, RP>::map(const V &f) {
	foreach([&](T &e) { e = f(e); });
}

template<typename T, typename RP>
template<typename U>
void Array<T, RP>::filter(const U &f) {
	Array<T, RP>::iterator it = begin();
	foreach([&](T &e) {
		if(f(reinterpret_cast<const T &>(e))) {
			*it = std::move(e);
			++it;
		}
	});
	dataEnd = it;
	shrinkIfNeeded();
}

template<typename T, typename RP>
template<typename U, typename C>
C Array<T, RP>::filtered(const U &f) const {
	C a;
	foreach([&](const T &e) {
		if(f(e)) {
			a.insert(e);
		}
	});
	return a;
}

template<typename T, typename RP>
template<typename U>
bool Array<T, RP>::forall(const U &f) const {
	for(const T &t : *this) {
		if(!f(t)) {
			return false;
		}
	}
	return true;
}

template<typename T, typename RP>
template<typename U>
typename Array<T, RP>::iterator Array<T, RP>::findOne(const U &f, const_iterator from, const_iterator to) {
	from = from ? from : begin();
	to = to ? to : end();
	for(iterator i = const_cast<iterator>(from); i != to; i++) {
		if(f(*i)) {
			return i;
		}
	}
	return end();
}

template<typename T, typename RP>
template<typename U>
typename Array<T, RP>::const_iterator Array<T, RP>::findOne(const U &f, const_iterator from, const_iterator to) const {
	from = from ? from : begin();
	to = to ? to : end();
	for(const_iterator i = from; i != to; i++) {
		if(f(*i)) {
			return i;
		}
	}
	return end();
}

template<typename T, typename RP>
typename Array<T, RP>::iterator Array<T, RP>::find(const T &e, const_iterator from, const_iterator to) {
	return findOne([&](const T &t) { return t == e; }, from, to);
}

template<typename T, typename RP>
typename Array<T, RP>::const_iterator Array<T, RP>::find(const T &e, const_iterator from, const_iterator to) const {
	return findOne([&](const T &t) { return t == e; }, from, to);
}

template<typename T, typename RP>
template<typename U>
uint Array<T, RP>::countAll(const U &f, const_iterator from, const_iterator to) const {
	from = from ? from : begin();
	to = to ? to : end();
	uint c = 0;
	for(const_iterator i = from; i != to; i++) {
		if(f(*i)) {
			c++;
		}
	}
	return c;
}

template<typename T, typename RP>
uint Array<T, RP>::count(const T &e, const_iterator from, const_iterator to) const {
	return countAll([&](const T &t) { return t == e; }, from, to);
}

template<typename T, typename RP>
template<typename V>
bool Array<T, RP>::existsOne(const V &f, const_iterator from, const_iterator to) const {
	from = from ? from : begin();
	to = to ? to : end();
	for(const_iterator i = from; i != to; i++) {
		if(f(*i)) {
			return true;
		}
	}
	return false;
}

template<typename T, typename RP>
bool Array<T, RP>::exists(const T &e, const_iterator from, const_iterator to) const {
	return existsOne([&](const T &t) { return t == e; }, from, to);
}

}
}
