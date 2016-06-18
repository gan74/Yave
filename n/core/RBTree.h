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

#ifndef N_CORE_RBTREE_H
#define N_CORE_RBTREE_H

#include <n/types.h>
#include <algorithm>
#include "Collection.h"

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
			template<typename J>
			IteratorBase(const J &it) : node(it.node) {
			}

			I &operator++() { // ++prefix
				node = next(node);
				return *reinterpret_cast<I *>(this);
			}

			I operator++(int) { // postfix++
				I it(*reinterpret_cast<I *>(this));
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

			const T *operator->() const {
				return &node->data;
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

	public:
		RBTree();
		RBTree(const RBTree<T, Comp, Eq> &o);
		RBTree(RBTree<T, Comp, Eq> &&o);

		template<typename C>
		RBTree(std::initializer_list<C> l);

		template<typename C, typename CC = typename std::enable_if<Collection<C>::isCollection>::type>
		RBTree(const C &c);


		~RBTree();


		const_iterator begin() const;
		const_iterator end() const;
		iterator begin();
		iterator end();
		const_iterator cbegin() const;
		const_iterator cend() const;


		template<typename U = T, typename C = Comp, typename E = Eq>
		iterator find(const U &t, const C &c = C(), const E &e = E());
		template<typename U = T, typename C = Comp, typename E = Eq>
		const_iterator find(const U &t, const C &c = C(), const E &e = E()) const;


		bool exists(const T &t) const;


		template<typename C>
		iterator insert(const C &c);


		void clear();


		iterator remove(iterator it);


		uint size() const;
		bool isEmpty() const;


		void swap(RBTree<T, Comp, Eq> &o);


		template<typename C>
		RBTree<T, Comp, Eq> &operator=(const C &o);
		RBTree<T, Comp, Eq> &operator=(const RBTree<T, Comp, Eq> &o);
		RBTree<T, Comp, Eq> &operator=(RBTree<T, Comp, Eq> &&o);


		RBTree<T, Comp, Eq> operator+(const RBTree<T, Comp, Eq> &e) const;


		template<typename C>
		RBTree<T, Comp, Eq> &operator+=(const C &e);
		template<typename C>
		RBTree<T, Comp, Eq> &operator<<(const C &e);


		template<typename C>
		bool operator==(const C &c) const;
		template<typename C>
		bool operator!=(const C &c) const;
		template<typename C>
		bool operator<(const C &c) const;


		template<typename U>
		void foreach(const U &f);
		template<typename U>
		void foreach(const U &f) const;


		template<typename U>
		bool forall(const U &f) const;


		template<typename V, typename C = RBTree<typename std::result_of<V(const T &)>::type>>
		C mapped(const V &f) const;
		template<typename V>
		void map(const V &f);


		template<typename U, typename C = RBTree<T, Comp, Eq>>
		C filtered(const U &f) const;
		template<typename U>
		void filter(const U &f);


	private:
		Comp comp;
		Eq eq;

		template<typename C>
		iterator insertDispatch(const C &c, TrueType) {
			return insertCollection(c);
		}

		template<typename C>
		iterator insertDispatch(const C &c, FalseType) {
			return insertOne(c);
		}

		Node *guard;
		Node *root;
		uint setSize;

		static Node *getMin(Node *n) {
			while(n->children[0]->color) {
				n = n->children[0];
			}
			return n;
		}

		/*static Node *getMax(Node *n) {
			while(n->children[1]->color) {
				n = n->children[1];
			}
			return n;
		}

		static Node *sibling(Node *n) {
			return n->parent->children[n->parent->children[0] == n];
		}*/

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
				if(m == end() || comp(*m, *it)) {
					m = it;
				}
			}
			return m;
		}

		template<typename C>
		iterator insertOne(const C &e) {
			if(!root->color) {
				guard->children[0] = guard->children[1] = root = new Node(T(e), guard, guard, guard);
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

template<typename T, typename Comp, typename Eq>
n::core::RBTree<T, Comp, Eq> operator+(const T &i, n::core::RBTree<T, Comp, Eq>  &a) {
	n::core::RBTree<T, Comp, Eq>  b(a);
	b.insert(i);
	return b;
}

template<typename T, typename Comp, typename Eq>
n::core::RBTree<T, Comp, Eq> operator+(const n::core::RBTree<T, Comp, Eq>  &a, const T &i) {
	n::core::RBTree<T, Comp, Eq>  b(a);
	b.insert(i);
	return b;
}

#include "RBTree_impl.h"


#endif // N_CORE_RBTREE_H
