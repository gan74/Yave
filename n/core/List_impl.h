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

template<typename T>
List<T>::iterator::iterator(const const_iterator &i) : iterator(i.elem) {
}

template<typename T>
List<T>::List() : lSize(0), tail(new ListElem()), head(tail) {
}

template<typename T>
template<typename C>
List<T>::List(std::initializer_list<C> l) : List() {
	for(auto x : l) {
		append(x);
	}
}

template<typename T>
template<typename C, typename CC>
List<T>::List(const C &c) : List() {
	for(auto x : c) {
		append(x);
	}
}

template<typename T>
List<T>::List(List<T> &&l) : List() {
	swap(l);
}

template<typename T>
List<T>::List(const List<T> &l) : List() {
	for(const T &x : l) {
		append(x);
	}
}

template<typename T>
List<T>::~List() {
	while(true) {
		ListElem *d = head;
		head = head->next;
		delete d;
		if(d == tail) {
			break;
		}
	}
}

template<typename T>
void List<T>::swap(List<T> &l) {
	ListElem *h = head;
	ListElem *t = tail;
	uint s = lSize;
	head = l.head;
	tail = l.tail;
	lSize = l.lSize;
	l.head = h;
	l.tail = t;
	l.lSize = s;
}

template<typename T>
void List<T>::swap(List<T> &&l) {
	swap(l);
}

template<typename T>
template<typename C>
void List<T>::append(const C &c) {
	appendDispatch(c, BoolToType<!ShouldInsertAsCollection<C, T>::value>());
}

template<typename T>
template<typename C>
void List<T>::append(std::initializer_list<C> c) {
	for(const auto &x : c) {
		append(x);
	}
}

template<typename T>
template<typename C>
void List<T>::prepend(const C &c) {
	prependDispatch(c, BoolToType<!ShouldInsertAsCollection<C, T>::value>());
}

template<typename T>
template<typename C>
void List<T>::insert(const C &c) {
	append(c);
}

template<typename T>
void List<T>::popFront() {
	ListElem *e = head->next;
	delete head;
	head = e;
	lSize--;
}

template<typename T>
void List<T>::pop() {
	remove(--end());
}

template<typename T>
void List<T>::move(const_iterator from, const_iterator to) {
	if(from == to) {
		return;
	}
	from.elem->prev->next = from.elem->next;
	from.elem->next->prev = from.elem->prev;

	from.elem->next = to.elem;
	from.elem->prev = to.elem->prev;
	to.elem->prev->next = from.elem;
	to.elem->prev = from.elem;
}

template<typename T>
typename List<T>::iterator List<T>::remove(const_iterator t) {
	if(t == begin()) {
		popFront();
		return begin();
	}
	lSize--;
	t.elem->next->prev = t.elem->prev;
	t.elem->prev->next = t.elem->next;
	ListElem *e = t.elem->next;
	delete t.elem;
	return iterator(e);
}

template<typename T>
template<typename C>
typename List<T>::iterator List<T>::insert(const C &e, const_iterator t) {
	return insertDispatch(e, t, BoolToType<ShouldInsertAsCollection<C, T>::value>());
}

template<typename T>
template<typename I>
typename List<T>::iterator List<T>::insert(I beg, I en, const_iterator pos) {
	for(I it = beg; it != en; ++it) {
		insert(*it, pos);
	}
	return pos;
}

template<typename T>
void List<T>::clear() {
	lSize = 0;
	ListElem *next = 0;
	for(ListElem *i = head; i != tail; i = next) {
		next = i.next;
		delete i;
	}
}

template<typename T>
template<typename C>
void List<T>::assign(const C &l) {
	if(&l != this) {
		assign(l.begin(), l.end());
	}
}

template<typename T>
template<typename I>
void List<T>::assign(I b, I e) {
	clear();
	for(I i = b; i != e; i++) {
		append(*i);
	}
}

template<typename T>
template<typename C>
List<T> &List<T>::operator=(const C &l) {
	assign(l);
	return *this;
}

template<typename T>
List<T> &List<T>::operator=(List<T> &&l) {
	swap(l);
	return *this;
}

template<typename T>
List<T> &List<T>::operator=(const List<T> &l) {
	if(&l != this) {
		assign(l);
	}
	return *this;
}

template<typename T>
uint List<T>::size() const {
	return lSize;
}

template<typename T>
bool List<T>::isEmpty() const {
	return !lSize;
}

template<typename T>
const T &List<T>::first() const {
	return *begin();
}

template<typename T>
const T &List<T>::last() const {
	return *(--end());
}

template<typename T>
T &List<T>::first() {
	return *begin();
}

template<typename T>
T &List<T>::last() {
	return *(--end());
}

template<typename T>
bool List<T>::isSorted() const {
	if(isEmpty()) {
		return true;
	}
	const_iterator l = begin();
	for(const_iterator it = ++begin(); it != end(); ++it) {
		if(*it < *l) {
			return false;
		}
		l = it;
	}
	return true;
}

template<typename T>
template<typename U>
typename List<T>::iterator List<T>::findOne(const U &f, const_iterator from) {
	for(iterator i = from; i != end(); ++i) {
		if(f(*i)) {
			return i;
		}
	}
	return end();
}

template<typename T>
template<typename U>
typename List<T>::const_iterator List<T>::findOne(const U &f, const_iterator from) const {
	for(const_iterator i = from; i != end(); ++i) {
		if(f(*i)) {
			return i;
		}
	}
	return end();
}

template<typename T>
template<typename U>
uint List<T>::countAll(const U &f) const {
	uint c = 0;
	for(const_iterator i = begin(); i != end(); ++i) {
		if(f(*i)) {
			c++;
		}
	}
	return c;
}

template<typename T>
template<typename V>
bool List<T>::existsOne(const V &f) const {
	for(const_iterator i = begin(); i != end(); ++i) {
		if(f(*i)) {
			return true;
		}
	}
	return false;
}

template<typename T>
typename List<T>::iterator List<T>::find(const T &e) {
	return findOne([&](const T &t) { return t == e; }, begin());
}

template<typename T>
typename List<T>::iterator List<T>::find(const T &e, const_iterator from) {
	return findOne([&](const T &t) { return t == e; }, from);
}

template<typename T>
typename List<T>::const_iterator List<T>::find(const T &e) const {
	return findOne([&](const T &t) { return t == e; }, begin());
}

template<typename T>
typename List<T>::const_iterator List<T>::find(const T &e, const_iterator from) const {
	return findOne([&](const T &t) { return t == e; }, from);
}

template<typename T>
uint List<T>::count(const T &e) const {
	return countAll([&](const T &t) { return t == e; });
}

template<typename T>
bool List<T>::exists(const T &e) const {
	return existsOne([&](const T &t) { return t == e; });
}

template<typename T>
template<typename U>
void List<T>::foreach(const U &f) {
	std::for_each(begin(), end(), f);
}

template<typename T>
template<typename U>
void List<T>::foreach(const U &f) const {
	std::for_each(begin(), end(), f);
}

template<typename T>
template<typename V, typename C>
C List<T>::mapped(const V &f) const {
	C a;
	foreach([&](const T &e) { a.insert(f(e)); });
	return a;
}

template<typename T>
template<typename U, typename C>
C List<T>::filtered(const U &f) const {
	C a;
	foreach([&](const T &e) {
		if(f(e)) {
			a.insert(e);
		}
	});
	return a;
}

template<typename T>
template<typename U>
bool List<T>::forall(const U &f) const {
	for(const T &t : *this) {
		if(!f(t)) {
			return false;
		}
	}
	return true;
}

template<typename T>
template<typename V>
void List<T>::map(const V &f) {
	foreach([&](T &e) { e = f(e); });
}

template<typename T>
template<typename U>
void List<T>::filter(const U &f) {
	for(iterator it = begin(); it != end();) {
		if(!f(*it)) {
			it = remove(it);
		} else {
			++it;
		}
	}
}

template<typename T>
typename List<T>::const_iterator List<T>::begin() const {
	return const_iterator(head);
}

template<typename T>
typename List<T>::const_iterator List<T>::end() const {
	return const_iterator(tail);
}

template<typename T>
typename List<T>::const_iterator List<T>::cbegin() const {
	return const_iterator(head);
}

template<typename T>
typename List<T>::const_iterator List<T>::cend() const {
	return const_iterator(tail);
}

template<typename T>
typename List<T>::iterator List<T>::begin() {
	return iterator(head);
}

template<typename T>
typename List<T>::iterator List<T>::end() {
	return iterator(tail);
}

template<typename T>
template<typename C>
bool List<T>::operator==(const C &l) const {
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

template<typename T>
template<typename C>
bool List<T>::operator!=(const C &l) const {
	return !operator==(l);
}

template<typename T>
template<typename C>
bool List<T>::operator<(const C &l) const {
	iterator a = begin();
	auto b = l.begin();
	while(a != end() && b != l.end()) {
		if(*a++ != *b++) {
			return false;
		}
	}
	return size() < l.size();
}

template<typename T>
template<typename C>
List<T> &List<T>::operator+=(const C &l) {
	append(l);
	return *this;
}

template<typename T>
template<typename C>
List<T> &List<T>::operator<<(const C &l) {
	append(l);
	return *this;
}

}
}
