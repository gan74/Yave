/*******************************
Copyright (C) 2013-2014 grégoire ANGERAND

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

#ifndef N_CORE_RBTREE_H
#define N_CORE_RBTREE_H

#include <n/Types.h>
#include <iostream>
#include <algorithm>

namespace n {
namespace core {

template<typename T, typename Comp = std::less<T>, typename Eq = std::equal_to<T>>
class RBTree
{
	class Node
	{
		public:
			static const byte Guard = 0;
			static const byte Black = 1;
			static const byte Red = 2;


			Node(const T &e, Node *p, Node *l, Node *r) : data(e), parent(p), children{l, r}, color(Red) {
			}

			Node() : parent(this), children{this, this}, color(Guard) {
			}

			~Node() {
				if(color != Guard) {
					data.~T();
				}
			}


			union
			{
				const T data;
			};

			Node *parent;
			Node *children[2];

			byte color;
	};

	template<typename I>
	class IteratorBase
	{
		public:
			IteratorBase(const I &it) : node(it.node) {
			}

			I &operator++() { // ++prefix
				node = next(node);
				return *(I *)this;
			}

			I operator++(int) { // postfix++
				I it(*(I *)this);
				operator++();
				return it;
			}

			I &operator--() { // --prefix
				node = prev(node);
				return *this;
			}

			I operator--(int) { // postfix--
				I it(*this);
				operator--();
				return it;
			}

			template<typename J>
			bool operator==(const IteratorBase<J> &it) const {
				return node == it.node;
			}

			template<typename J>
			bool operator!=(const IteratorBase<J> &it) const {
				return node != it.node;
			}

			const T &operator*() const {
				return node->data;
			}

		protected:
			friend class RBTree;
			IteratorBase(Node *n) : node(n) {
			}


			Node *node;
	};

	public:
		class iterator : public IteratorBase<iterator>
		{
			public:
				iterator(const iterator &it) : IteratorBase<iterator>(it) {
				}

			private:
				friend class RBTree;
				iterator(Node *n) : IteratorBase<iterator>(n) {
				}
		};

		class const_iterator : public IteratorBase<const_iterator>
		{
			public:
				const_iterator(const const_iterator &it) : IteratorBase<const_iterator>(it) {
				}

				const_iterator(const iterator &it) : IteratorBase<const_iterator>(it) {
				}

			private:
				friend class RBTree;
				const_iterator(Node *n) : IteratorBase<const_iterator>(n) {
				}
		};

	private:
		template<bool C, typename U>
		struct Adder
		{
			RBTree<T, Comp, Eq>::iterator operator()(RBTree<T, Comp, Eq> &set, const U &e) {
				return set.insertOne(e);
			}
		};

		template<typename U>
		struct Adder<false, U>
		{
			RBTree<T, Comp, Eq>::iterator operator()(RBTree<T, Comp, Eq> &set, const U &e) {
				return set.insertCollection(e);
			}
		};

	public:
		RBTree() : guard(new Node()), root(guard), setSize(0) {
		}

		RBTree(const RBTree<T, Comp, Eq> &o) : RBTree() {
			for(const T &e : o) {
				insertAtEnd(e);
			}
		}

		RBTree(RBTree<T, Comp, Eq> &&o) : RBTree() {
			swap(std::move(o));
		}

		~RBTree() {
			clear();
			delete guard;
		}

		const_iterator begin() const {
			return guard->children[1];
		}

		const_iterator end() const {
			return guard;
		}

		iterator begin() {
			return guard->children[1];
		}

		iterator end() {
			return guard;
		}

		const_iterator cbegin() const {
			return begin();
		}

		const_iterator cend() const {
			return end();
		}

		template<typename U = T, typename C = Comp, typename E = Eq>
		iterator find(const U &t, const C c = C(), const E e = E()) {
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

		template<typename U = T, typename C = Comp, typename E = Eq>
		const_iterator find(const U &t, const C c = C(), const E e = E()) const {
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

		template<typename C>
		iterator insert(const C &c) {
			return Adder<TypeConversion<C, T>::exists, C>()(*this, c);
		}

		void clear() {
			clearOne(root);
			root = guard->children[0] = guard->children[1] = guard->parent = guard;
			setSize = 0;
		}

		iterator remove(iterator it) {
			Node *z = it.node;
			if(!z->color) {
				return end();
			}
			setSize--;
			Node *td = z;
			it++;
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
					x->parent = z;
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

		uint size() const {
			return setSize;
		}

		void swap(RBTree<T, Comp, Eq> &&o) {
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

		template<typename C>
		RBTree<T, Comp, Eq> &operator=(const C &o) {
			clear();
			insert(o);
			return *this;
		}

		RBTree<T, Comp, Eq> &operator=(RBTree<T, Comp, Eq> &&o) {
			swap(std::move(o));
			return *this;
		}

		template<typename C>
		RBTree<T, Comp, Eq> operator+(const C &e) const {
			RBTree<T, Comp, Eq> a(*this);
			for(const T &i : e) {
				a.insert(i);
			}
			return a;
		}

		template<typename C>
		RBTree<T, Comp, Eq> &operator+=(const C &e) {
			insert(e);
			return *this;
		}

		template<typename C>
		RBTree<T, Comp, Eq> &operator<<(const C &e) {
			insert(e);
			return *this;
		}

		template<typename C>
		bool operator==(const C &c) const {
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

		template<typename C>
		bool operator!=(const C &c) const {
			return !operator==(c);
		}

		template<typename C>
		bool operator<(const C &c) const {
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

		template<typename V, typename C = RBTree<typename std::result_of<V(const T &)>::type, Comp, Eq>>
		C mapped(const V &f) const {
			RBTree<typename std::result_of<V(const T &)>::type, Comp, Eq> a;
			foreach([&](const T &e) { a.insert(f(e)); });
			return a;
		}

		template<typename U, typename C = RBTree<T, Comp, Eq>>
		C filtered(const U &f) const {
			RBTree<T, Comp, Eq> a;
			foreach([&](const T &e) {
				if(f(e)) {
					a.insertAtEnd(e);
				}
			});
			return a;
		}

		template<typename U>
		void filter(const U &f) {
			for(iterator it = begin(); it != end();) {
				if(!f(*it)) {
					it = remove(it);
				} else {
					it++;
				}
			}
		}

		template<typename V, typename U>
		V foldRight(const U &f, V def = V(0)) {
			V acc(def);
			foreach([&](const T &t) { acc = f(t, acc); });
			return acc;
		}

	private:
		Node *guard;
		Node *root;
		uint setSize;

		static Node *getMin(Node *n) {
			while(n->children[0]->color) {
				n = n->children[0];
			}
			return n;
		}

		static Node *getMax(Node *n) {
			while(n->children[1]->color) {
				n = n->children[1];
			}
			return n;
		}

		static Node *sibling(Node *n) {
			return n->parent->children[n->parent->children[0] == n];
		}

		static Node *next(Node *x) {
			if(x->children[1]->color) {
				return getMin(x->children[1]);
			}
			Node *y = x->parent;
			while(y->color && x == y->children[1]) {
				x = y;
				y = y->parent;
			}
			return y;
		}

		static Node *prev(Node *x) {
			if(x->children[0]->color) {
				return getMin(x->children[0]);
			}
			Node *y = x->parent;
			while(y->color && x == y->children[0]) {
				x = y;
				y = y->parent;
			}
			return y;
		}

		static void clearOne(Node *x) {
			if(x->color) {
				clearOne(x->children[0]);
				clearOne(x->children[1]);
				delete x;
			}
		}

		void leftRot(Node *x) {
			Node *y = x->children[1];
			x->children[1] = y->children[0];
			if(y->children[0]->color) {
				y->children[0]->parent = x;
			}
			y->parent = x->parent;
			if(!x->parent->color) {
				root = y;
			} else if(x == x->parent->children[0]) {
				x->parent->children[0] = y;
			} else {
				x->parent->children[1] = y;
			}
			y->children[0] = x;
			x->parent = y;
		}

		void rightRot(Node *x) {
			Node *y = x->children[0];
			x->children[0] = y->children[1];
			if(y->children[1]->color) {
				y->children[1]->parent = x;
			}
			y->parent = x->parent;
			if(!x->parent->color) {
				root = y;
			} else if(x == x->parent->children[1]) {
				x->parent->children[1] = y;
			} else {
				x->parent->children[0] = y;
			}
			y->children[1] = x;
			x->parent = y;
		}

		void transplant(Node *u, Node *v) {
			if(!u->parent->color) {
				root = v;
			} else if(u == u->parent->children[0]) {
				u->parent->children[0] = v;
			} else {
				u->parent->children[1] = v;
			}
			v->parent = u->parent;
		}

		void insertAtEnd(const T &e) {
			if(!root->color)  {
				insert(e);
			} else {
				Node *x = prev(guard);
				RBinsert(guard->children[0] = x->children[1] = new Node(e, x, guard, guard));
				setSize++;
			}
		}

		void RBinsert(Node *z) {
			Node *y = guard;
			while(z->parent->color == Node::Red) {
				bool l = (z->parent == z->parent->parent->children[0]); {
					y = z->parent->parent->children[l];
					if(y->color == Node::Red) {
						z->parent->color = Node::Black;
						y->color = Node::Black;
						z->parent->parent->color = Node::Red;
						z = z->parent->parent;
					} else {
						if(z == z->parent->children[l]) {
							z = z->parent;
							if(l) {
								leftRot(z);
							} else {
								rightRot(z);
							}
						}
						z->parent->color = Node::Black;
						z->parent->parent->color = Node::Red;
						if(l) {
							rightRot(z->parent->parent);
						} else {
							leftRot(z->parent->parent);
						}
					}
				}
			}
			root->color = Node::Black;
			guard->color = Node::Guard;
		}

		void RBremove(Node *x) {
			while(x != root && x->color != Node::Red) {
				bool l = x == x->parent->children[0];
				Node *w = x->parent->children[l];
				if(w->color == Node::Red) {
					w->color = Node::Black;
					x->parent->color = Node::Red;
					if(l) {
						leftRot(x->parent);
					} else {
						rightRot(x->parent);
					}
					w = x->parent->children[l];
				}
				if(w->children[0]->color != Node::Red && w->children[1]->color != Node::Red) {
					w->color = Node::Red;
					x = x->parent;
				} else {
					if(w->children[l]->color != Node::Red) {
						w->children[!l]->color = Node::Black;
						w->color = Node::Red;
						if(l) {
							rightRot(w);
						} else {
							leftRot(w);
						}
						w = x->parent->children[l];
					}
					w->color = x->parent->color;
					x->parent->color = Node::Black;
					w->children[l]->color = Node::Black;
					if(l) {
						leftRot(x->parent);
					} else {
						rightRot(x->parent);
					}
					x = root;
				}
			}
			x->color = Node::Black;
			root->color = Node::Black;
			guard->color = Node::Guard;
		}

			template<typename C>
		iterator insertCollection(const C &c) {
			iterator m = end();
			for(const auto &e : c) {
				iterator it = insert(e);
				if(m == end() || *m < *it) {
					m = it;
				}
			}
			return m;
		}

		iterator insertOne(const T &e) {
			Comp comp;
			Eq eq;
			if(!root->color) {
				guard->children[0] = guard->children[1] = root = new Node(e, guard, guard, guard);
				root->color = Node::Black;
				setSize = 1;
				return iterator(root);
			}
			Node *n = root;
			Node *l = guard;
			uint o = 0;
			int w = 0;
			int d = 0;
			while(n->color) {
				l = n;
				if(eq(e, n->data)) {
					return end();
				}
				d++;
				if(comp(e, n->data)) {
					n = n->children[o = 0];
					w++;
				} else {
					n = n->children[o = 1];
					w--;
				}
			}
			n = l->children[o] = new Node(e, l, guard, guard);
			RBinsert(n);
			if(w == d) {
				guard->children[1] = n;
			} else if(w == -d) {
				guard->children[0] = n;
			}
			setSize++;
			return iterator(n);
		}

};

}
}

#endif // N_CORE_RBTREE_H
