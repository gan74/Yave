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

namespace n {
namespace core {

template<typename T, typename Comp = std::less<T>>
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

	public:
		class const_iterator
		{
			public:
				const_iterator &operator++() { // ++prefix
					node = next(node);
					return *this;
				}

				const_iterator operator++(int) { // postfix++
					const_iterator it(*this);
					operator++();
					return it;
				}

				const_iterator &operator--() { // --prefix
					node = prev(node);
					return *this;
				}

				const_iterator operator--(int) { // postfix--
					const_iterator it(*this);
					operator--();
					return it;
				}

				bool operator==(const const_iterator &it) const {
					return node == it.node;
				}

				bool operator!=(const const_iterator &it) const {
					return node != it.node;
				}

				const T &operator*() const {
					return node->data;
				}

			public:
				const_iterator(Node *n) : node(n) {
				}

				Node *node;
		};

		typedef const_iterator iterator;

		RBTree() : guard(new Node()), root(guard), setSize(0) {
		}

		RBTree(const RBTree<T> &o) : RBTree() {
			for(const T &e : o) {
				insert(e);
			}
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

		const_iterator find(const T &e) const {
			Node *n = root;
			while(n->color) {
				if(n->data == e) {
					return const_iterator(n);
				}
				if(comp(e, n->data)) {
					n = n->children[0];
				} else {
					n = n->children[1];
				}
			}
			return end();
		}

		const_iterator insert(const T &e) {
			if(!root->color) {
				root = new Node(e, guard, guard, guard);
				root->color = Node::Black;
				setSize = 1;
				return const_iterator(root);
			}
			Node *n = root;
			Node *l = guard;
			uint o = 0;
			int w = 0;
			int d = 0;
			while(n->color) {
				l = n;
				if(n->data == e) {
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
			return const_iterator(n);
		}

		void clear() {
			clearOne(root);
			root = guard->children[0] = guard->children[1] = guard->parent = guard;
			setSize = 0;
		}

		const_iterator remove(const_iterator it) {
			setSize--;
			Node *z = it.node;
			Node *td = z;
			it++;
			if(!z->color) {
				return end();
			}
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

		/*void print() {
			printOne(root, 0);
		}

		void printOne(Node *z, uint t) {
			if(z->color) {
				for(uint i = 0; i != t; i++) {
					std::cout<<" ";
				}
				std::cout<<z->data<<"  ";
				if(z->color == Node::Red) {
					std::cout<<"(R)";
				} else {
					std::cout<<"(b)";
				}
				std::cout<<std::endl;
				printOne(z->children[0], t + 1);
				printOne(z->children[1], t + 1);
			}
		}

		uint computeHeight(Node *x = 0) {
			if(!x) {
				x = root;
			}
			if(x->color) {
				return std::max(computeHeight(x->children[0]), computeHeight(x->children[1])) + 1;
			}
			return 0;
		}*/

		RBTree<T> &operator=(const RBTree<T> &o) {
			clear();
			for(const T &e : o) {
				insert(e);
			}
			return *this;
		}

		RBTree<T> operator+(const T &e) const {
			RBTree<T> a(*this);
			a.insert(e);
			return a;
		}

		RBTree<T> operator+(const RBTree<T> &e) const {
			RBTree<T> a(*this);
			for(const T &i : e) {
				a.insert(i);
			}
			return a;
		}

		RBTree<T> &operator+=(const T &e) {
			insert(e);
			return *this;
		}

		RBTree<T> &operator+=(const RBTree<T> &e) {
			for(const T &i : e) {
				insert(i);
			}
			return *this;
		}

		RBTree<T> &operator<<(const T &e) {
			insert(e);
			return *this;
		}

		RBTree<T> &operator<<(const RBTree<T> &e) {
			for(const T &i : e) {
				insert(i);
			}
			return *this;
		}

		bool operator==(const RBTree<T> &tree) const {
			if(size() == tree.size()) {
				const_iterator a = begin();
				const_iterator b = tree.begin();
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

		bool operator!=(const RBTree<T> &tree) const {
			return !operator==(tree);
		}

		bool operator<(const RBTree<T> &tree) const {
			const_iterator a = begin();
				const_iterator b = tree.begin();
				while(a != end() && b != tree.end()) {
					if(*a != *b) {
						return false;
					}
					++a;
					++b;
				}
			return size() < tree.size();
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
		RBTree<typename std::result_of<V(const T &)>::type> mapped(const V &f) const {
			RBTree<typename std::result_of<V(const T &)>::type> a;
			foreach([&](const T &e) { a.insert(f(e)); });
			return a;
		}

		template<typename U>
		RBTree<T> filtered(const U &f) const {
			RBTree<T> a;
			foreach([&](const T &e) {
				if(f(e)) {
					a.insert(e);
				}
			});
			return a;
		}

		template<typename U>
		void filter(const U &f) {
			for(const_iterator it = begin(); it != end();) {
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
		Comp comp;
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

};

}
}

#endif // N_CORE_RBTREE_H
