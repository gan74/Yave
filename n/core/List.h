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

#ifndef N_CORE_LIST_H
#define N_CORE_LIST_H

#include <n/Types.h>

namespace n {
namespace core {

template<typename T>
class List
{
	class ListElem
	{
		public:
			ListElem(const T &t, ListElem *n = 0, ListElem *p = 0) : ListElem(n, p) {
				new(&elem) T(t);
			}

			ListElem(ListElem *n = 0, ListElem *p = 0) : next(n ? n : this), prev(p ? p : this) {
			}

			union
			{
				T elem;
			};
			ListElem *next;
			ListElem *prev;
	};

	template<typename... Args>
	void construct(const T &a, const Args&... args) {
		append(a);
		construct(args...);
	}

	void construct(const T &a) {
		append(a);
	}

	public:
		class iterator
		{
			public:
				iterator &operator++() {
					elem = elem->next;
					return *this;
				}

				iterator &operator--() {
					elem = elem->prev;
					return *this;
				}

				iterator operator++(int) {
					iterator it(elem);
					elem = elem->next;
					return it;
				}

				iterator operator--(int) {
					iterator it(elem);
					elem = elem->prev;
					return it;
				}

				bool operator!=(const iterator &t) const {
					return t.elem != elem;
				}

				bool operator==(const iterator &t) const {
					return t.elem == elem;
				}

				T &operator*() {
					return elem->elem;
				}

				const T &operator*() const {
					return elem->elem;
				}

			private:
				friend class List;
				iterator(ListElem *e) : elem(e) {
				}


				template<typename C>
				iterator(const C &i) : iterator(i.elem) {
				}


				ListElem *elem;
		};

		class const_iterator
		{
			public:
				const_iterator &operator++() {
					elem = elem->next;
					return *this;
				}

				const_iterator &operator--() {
					elem = elem->prev;
					return *this;
				}

				const_iterator operator++(int) {
					const_iterator it(elem);
					elem = elem->next;
					return it;
				}

				const_iterator operator--(int) {
					const_iterator it(elem);
					elem = elem->prev;
					return it;
				}

				bool operator!=(const const_iterator &t) const {
					return t.elem != elem;
				}

				bool operator==(const const_iterator &t) const {
					return t.elem == elem;
				}

				const T &operator*() const {
					return elem->elem;
				}

				const_iterator(const iterator &i) : const_iterator(i.elem) {
				}

			private:
				friend class List;
				const_iterator(ListElem *e) : elem(e) {
				}

				ListElem *elem;
		};

		List() : lSize(0), tail(new ListElem()), head(tail) {
		}

		template<typename... Args>
		List(const T & a, const Args&... args) : List() {
			construct(a, args...);
		}

		List<T> &append(const T &t) {
			ListElem *e = new ListElem(t, tail, 0);
			if(head == tail) {
				head =  e;
			} else {
				tail->prev->next = e;
				e->prev = tail->prev;
			}
			tail->prev = e;
			lSize++;
			return *this;
		}

		List<T> &prepend(const T &t) {
			if(isEmpty()) {
				append(t);
			} else {
				ListElem *e = new ListElem(t, head, 0);
				head = head->prev = e;
				lSize++;
			}
			return *this;
		}

		void popFront() {
			ListElem *e = head->next;
			delete head;
			head = e;
			lSize--;
		}

		iterator remove(const iterator &t) {
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

		iterator insert(const T &e, const const_iterator &t) {
			if(t == begin()) {
				prepend(e);
				return begin();
			}
			lSize++;
			ListElem *el = new ListElem(e, t.elem, t.elem->prev);
			el->next->prev = el;
			el->prev->next = el;
			return iterator(el);
		}

		iterator insert(const const_iterator &beg, const const_iterator &en, iterator pos) {
			for(iterator it = beg; it != en; it++) {
				insert(*it, pos);
			}
			return pos;
		}

		void clear() {
			lSize = 0;
			ListElem *next = 0;
			for(ListElem *i = head; i != tail; i = next) {
				next = i.next;
				delete i;
			}
		}

		void assign(const List<T> &l) {
			if(&l != this) {
				assign(l.begin(), l.end());
			}
		}

		void assign(const const_iterator &b, const const_iterator &e) {
			clear();
			for(const_iterator i = b; i != e; i++) {
				append(*i);
			}
		}

		List<T> &operator=(const List<T> &l) {
			assign(l);
		}

		uint size() const {
			return lSize;
		}

		bool isEmpty() const {
			return !lSize;
		}

		const T &first() const {
			return *begin();
		}

		const T &last() const {
			return *(--end());
		}

		T &first() {
			return *begin();
		}

		T &last() {
			return *(--end());
		}

		template<typename U>
		iterator find(const U &f, const const_iterator &from) {
			for(iterator i = from; i != end(); i++) {
				if(f(*i)) {
					return i;
				}
			}
			return end();
		}

		template<typename U>
		const_iterator find(const U &f, const const_iterator &from) const {
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
			for(const_iterator it = ++begin(); it != end(); it++) {
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

		iterator find(const T &e) {
			return find([&](const T &t) { return t == e; }, begin());
		}

		iterator find(const T &e, const const_iterator &from) {
			return find([&](const T &t) { return t == e; }, from);
		}

		const_iterator find(const T &e) const {
			return find([&](const T &t) { return t == e; }, begin());
		}

		const_iterator find(const T &e, const const_iterator &from) const {
			return find([&](const T &t) { return t == e; }, from);
		}

		uint count(const T &e) const {
			return count([&](const T &t) { return t == e; });
		}

		const_iterator begin() const {
			return const_iterator(head);
		}

		const_iterator end() const {
			return const_iterator(tail);
		}

		const_iterator cbegin() const {
			return const_iterator(head);
		}

		const_iterator cend() const {
			return const_iterator(tail);
		}

		iterator begin() {
			return iterator(head);
		}

		iterator end() {
			return iterator(tail);
		}

		bool operator==(const List<T> &l) const {
			if(size() == l.size()) {
				const_iterator a = begin();
				const_iterator b = l.begin();
				while(a != end()) {
					if(*a++ != *b++) {
						return false;
					}
				}
				return true;
			}
			return false;
		}

		bool operator<(const List<T> &l) const {
			iterator a = begin();
			iterator b = l.begin();
			while(a != end() && b != l.end()) {
				if(*a++ != *b++) {
					return false;
				}
			}
			return size() < l.size();
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
		List<T> mapped(const V &f) const {
			List<T> a;
			foreach([&](const T &e) { a.append(f(e)); });
			return a;
		}

		template<typename U>
		void map(const U &f) {
			foreach([&](T &e) { e = f(e); });
		}

		template<typename U>
		List<T> filtered(const U &f) const {
			List<T> a;
			foreach([&](const T &e) {
				if(f(e)) {
					a.append(e);
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
		uint lSize;
		ListElem *tail;
		ListElem *head;

};

}
}

#endif // N_CORE_LIST_H
