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

template<typename T, typename Comp, typename Eq>
RBTree<T, Comp, Eq>::RBTree() : guard(new Node()), root(guard), setSize(0) {
}

template<typename T, typename Comp, typename Eq>
RBTree<T, Comp, Eq>::RBTree(const RBTree<T, Comp, Eq> &o) : RBTree() {
	for(const T &e : o) {
		insertAtEnd(e);
	}
}

template<typename T, typename Comp, typename Eq>
RBTree<T, Comp, Eq>::RBTree(RBTree<T, Comp, Eq> &&o) : RBTree() {
	swap(o);
}

template<typename T, typename Comp, typename Eq>
template<typename C>
RBTree<T, Comp, Eq>::RBTree(std::initializer_list<C> l) : RBTree() {
	for(auto x : l) {
		insert(x);
	}
}

template<typename T, typename Comp, typename Eq>
template<typename C, typename CC>
RBTree<T, Comp, Eq>::RBTree(const C &c) : RBTree() {
	for(auto x : c) {
		insert(x);
	}
}

template<typename T, typename Comp, typename Eq>
RBTree<T, Comp, Eq>::~RBTree() {
	clear();
	delete guard;
}

template<typename T, typename Comp, typename Eq>
typename RBTree<T, Comp, Eq>::const_iterator RBTree<T, Comp, Eq>::begin() const {
	return guard->children[1];
}

template<typename T, typename Comp, typename Eq>
typename RBTree<T, Comp, Eq>::const_iterator RBTree<T, Comp, Eq>::end() const {
	return guard;
}

template<typename T, typename Comp, typename Eq>
typename RBTree<T, Comp, Eq>::iterator RBTree<T, Comp, Eq>::begin() {
	return guard->children[1];
}

template<typename T, typename Comp, typename Eq>
typename RBTree<T, Comp, Eq>::iterator RBTree<T, Comp, Eq>::end() {
	return guard;
}

template<typename T, typename Comp, typename Eq>
typename RBTree<T, Comp, Eq>::const_iterator RBTree<T, Comp, Eq>::cbegin() const {
	return begin();
}

template<typename T, typename Comp, typename Eq>
typename RBTree<T, Comp, Eq>::const_iterator RBTree<T, Comp, Eq>::cend() const {
	return end();
}

template<typename T, typename Comp, typename Eq>
template<typename U, typename C, typename E>
typename RBTree<T, Comp, Eq>::iterator RBTree<T, Comp, Eq>::find(const U &t, const C &c, const E &e) {
	Node *n = root;
	while(n->color) {
		if(e(t, n->data)) {
			return iterator(n);
		}
		if(c(t, n->data)) {
			n = n->children[0];
		} else {
			n = n->children[1];
		}
	}
	return end();
}

template<typename T, typename Comp, typename Eq>
template<typename U, typename C, typename E>
typename RBTree<T, Comp, Eq>::const_iterator RBTree<T, Comp, Eq>::find(const U &t, const C &c, const E &e) const {
	Node *n = root;
	while(n->color) {
		if(e(t, n->data)) {
			return const_iterator(n);
		}
		if(c(t, n->data)) {
			n = n->children[0];
		} else {
			n = n->children[1];
		}
	}
	return end();
}

template<typename T, typename Comp, typename Eq>
bool RBTree<T, Comp, Eq>::exists(const T &t) const {
	return find(t) != end();
}

template<typename T, typename Comp, typename Eq>
template<typename C>
typename RBTree<T, Comp, Eq>::iterator RBTree<T, Comp, Eq>::insert(const C &c) {
	return insertDispatch(c, typename ShouldInsertAsCollection<C, T>::type());
}

template<typename T, typename Comp, typename Eq>
void RBTree<T, Comp, Eq>::clear() {
	clearOne(root);
	root = guard->children[0] = guard->children[1] = guard->parent = guard;
	setSize = 0;
}

template<typename T, typename Comp, typename Eq>
typename RBTree<T, Comp, Eq>::iterator RBTree<T, Comp, Eq>::remove(iterator it) {
	Node *z = it.node;
	if(!z->color) {
		return end();
	}
	setSize--;
	Node *td = z;
	++it;
	for(uint i = 0; i != 2; i++) {
		if(z == guard->children[i]) {
			guard->children[i] = z->parent;
		}
	}
	Node *y = z;
	Node *x = guard;
	byte color = y->color;
	if(!z->children[0]->color) {
		x = z->children[1];
		transplant(z, z->children[1]);
	} else if(!z->children[1]->color) {
		x = z->children[0];
		transplant(z, z->children[0]);
	} else {
		y = getMin(z->children[1]);
		color = y->color;
		x = y->children[1];
		if(y->parent == z) {
			x->parent = y;
		} else {
			transplant(y, y->children[1]);
			y->children[1] = z->children[1];
			y->children[1]->parent = y;
		}
		transplant(z, y);
		y->children[0] = z->children[0];
		y->children[0]->parent = y;
		y->color = z->color;
	}
	if(color != Node::Red) {
		RBremove(x);
	}
	delete td;
	return it;
}

template<typename T, typename Comp, typename Eq>
uint RBTree<T, Comp, Eq>::size() const {
	return setSize;
}

template<typename T, typename Comp, typename Eq>
bool RBTree<T, Comp, Eq>::isEmpty() const {
	return !setSize;
}

template<typename T, typename Comp, typename Eq>
void RBTree<T, Comp, Eq>::swap(RBTree<T, Comp, Eq> &o) {
	Node *r = o.root;
	Node *g = o.guard;
	uint s = o.setSize;
	o.root = root;
	o.guard = guard;
	o.setSize = setSize;
	root = r;
	guard = g;
	setSize = s;
}

template<typename T, typename Comp, typename Eq>
RBTree<T, Comp, Eq> &RBTree<T, Comp, Eq>::operator=(const RBTree<T, Comp, Eq> &o) {
	if(&o != this) {
		clear();
		insert(o);
	}
	return *this;
}

template<typename T, typename Comp, typename Eq>
template<typename C>
RBTree<T, Comp, Eq> &RBTree<T, Comp, Eq>::operator=(const C &o) {
	clear();
	insert(o);
	return *this;
}

template<typename T, typename Comp, typename Eq>
RBTree<T, Comp, Eq> &RBTree<T, Comp, Eq>::operator=(RBTree<T, Comp, Eq> &&o) {
	swap(o);
	return *this;
}

template<typename T, typename Comp, typename Eq>
RBTree<T, Comp, Eq> RBTree<T, Comp, Eq>::operator+(const RBTree<T, Comp, Eq> &e) const {
	RBTree<T, Comp, Eq> a(*this);
	for(const T &i : e) {
		a.insert(i);
	}
	return a;
}

template<typename T, typename Comp, typename Eq>
template<typename C>
RBTree<T, Comp, Eq> &RBTree<T, Comp, Eq>::operator+=(const C &e) {
	insert(e);
	return *this;
}

template<typename T, typename Comp, typename Eq>
template<typename C>
RBTree<T, Comp, Eq> &RBTree<T, Comp, Eq>::operator<<(const C &e) {
	insert(e);
	return *this;
}

template<typename T, typename Comp, typename Eq>
template<typename C>
bool RBTree<T, Comp, Eq>::operator==(const C &c) const {
	if(size() == c.size()) {
		const_iterator a = begin();
		const_iterator b = c.begin();
		while(a != end()) {
			if(*a != *b) {
				return false;
			}
			++a;
			++b;
		}
		return true;
	}
	return false;
}

template<typename T, typename Comp, typename Eq>
template<typename C>
bool RBTree<T, Comp, Eq>::operator!=(const C &c) const {
	return !operator==(c);
}

template<typename T, typename Comp, typename Eq>
template<typename C>
bool RBTree<T, Comp, Eq>::operator<(const C &c) const {
	const_iterator a = begin();
		const_iterator b = c.begin();
		while(a != end() && b != c.end()) {
			if(*a != *b) {
				return false;
			}
			++a;
			++b;
		}
	return size() < c.size();
}

template<typename T, typename Comp, typename Eq>
template<typename U>
void RBTree<T, Comp, Eq>::foreach(const U &f) {
	std::for_each(begin(), end(), f);
}

template<typename T, typename Comp, typename Eq>
template<typename U>
void RBTree<T, Comp, Eq>::foreach(const U &f) const {
	std::for_each(begin(), end(), f);
}

template<typename T, typename Comp, typename Eq>
template<typename U>
bool RBTree<T, Comp, Eq>::forall(const U &f) const {
	for(const T &t : *this) {
		if(!f(t)) {
			return false;
		}
	}
	return true;
}

template<typename T, typename Comp, typename Eq>
template<typename V, typename C>
C RBTree<T, Comp, Eq>::mapped(const V &f) const {
	RBTree<typename std::result_of<V(const T &)>::type> a;
	foreach([&](const T &e) { a.insert(f(e)); });
	return a;
}

template<typename T, typename Comp, typename Eq>
template<typename V>
void RBTree<T, Comp, Eq>::map(const V &f) {
	operator=(mapped(f));
}

template<typename T, typename Comp, typename Eq>
template<typename U, typename C>
C RBTree<T, Comp, Eq>::filtered(const U &f) const {
	C a;
	foreach([&](const T &e) {
		if(f(e)) {
			a.insert(e);
		}
	});
	return a;
}

template<typename T, typename Comp, typename Eq>
template<typename U>
void RBTree<T, Comp, Eq>::filter(const U &f) {
	for(iterator it = begin(); it != end();) {
		if(!f(*it)) {
			it = remove(it);
		} else {
			++it;
		}
	}
}

}
}
