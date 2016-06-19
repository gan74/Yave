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
#ifndef N_CORE_LIST_H
#define N_CORE_LIST_H

#include <n/types.h>
#include <n/utils.h>
#include "Collection.h"

namespace n {
namespace core {

template<typename T>
class List
{
	class ListElem : NonCopyable
	{
		public:
			ListElem(const T &t, ListElem *n = 0, ListElem *p = 0) : ListElem(n, p) {
				new(&elem) T(t);
			}

			ListElem(ListElem *n = 0, ListElem *p = 0) : next(n ? n : this), prev(p ? p : this) {
			}

			~ListElem() {
				if(next != this) {
					elem.~T();
				}
			}

			union
			{
				T elem;
			};
			ListElem *next;
			ListElem *prev;
	};

	public:
		class const_iterator;

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

				bool operator!=(iterator t) const {
					return t.elem != elem;
				}

				bool operator==(iterator t) const {
					return t.elem == elem;
				}

				T &operator*() const {
					return elem->elem;
				}

				T *operator->() const {
					return elem->elem;
				}

				iterator(const iterator &i) : iterator(i.elem) {
				}

			private:
				friend class List;
				iterator(ListElem *e) : elem(e) {
				}

				iterator(const const_iterator &i);

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

				bool operator!=(const_iterator t) const {
					return t.elem != elem;
				}

				bool operator==(const_iterator t) const {
					return t.elem == elem;
				}

				const T &operator*() const {
					return elem->elem;
				}

				const T *operator->() const {
					return &elem->elem;
				}

				const_iterator(const iterator &i) : const_iterator(i.elem) {
				}

				const_iterator(const const_iterator &i) : const_iterator(i.elem) {
				}

			private:
				friend class List;
				const_iterator(ListElem *e) : elem(e) {
				}

				ListElem *elem;
		};


		using Element = T;

		List();
		List(List<T> &&l);
		List(const List<T> &l);

		template<typename C>
		List(std::initializer_list<C> l);

		template<typename C, typename CC = typename std::enable_if<Collection<C>::isCollection>::type>
		List(const C &c);


		~List();


		void swap(List<T> &l);
		void swap(List<T> &&l);

		template<typename C>
		void append(const C &c);
		template<typename C>
		void append(std::initializer_list<C> c);


		template<typename C>
		void prepend(const C &c);


		template<typename C>
		void insert(const C &c);
		template<typename C>
		iterator insert(const C &e, const_iterator t);
		template<typename I>
		iterator insert(I beg, I en, const_iterator pos);


		void popFront();
		void pop();


		void move(const_iterator from, const_iterator to);


		iterator remove(const_iterator t);


		void clear();


		template<typename C>
		void assign(const C &l);
		template<typename I>
		void assign(I b, I e);


		template<typename C>
		List<T> &operator=(const C &l);
		List<T> &operator=(List<T> &&l);
		List<T> &operator=(const List<T> &l);


		uint size() const;
		bool isEmpty() const;


		const T &first() const;
		const T &last() const;
		T &first();
		T &last();


		bool isSorted() const;



		template<typename U>
		iterator findOne(const U &f, const_iterator from);
		iterator find(const T &e);
		iterator find(const T &e, const_iterator from);

		template<typename U>
		const_iterator findOne(const U &f, const_iterator from) const;
		const_iterator find(const T &e) const;
		const_iterator find(const T &e, const_iterator from) const;


		template<typename U>
		uint countAll(const U &f) const;

		uint count(const T &e) const;


		template<typename V>
		bool existsOne(const V &f) const;

		bool exists(const T &e) const;


		template<typename U>
		void foreach(const U &f);
		template<typename U>
		void foreach(const U &f) const;


		template<typename U>
		bool forall(const U &f) const;

		template<typename V>
		void map(const V &f);
		template<typename V, typename C = List<typename std::result_of<V(const T &)>::type>>
		C mapped(const V &f) const;


		template<typename U>
		void filter(const U &f);
		template<typename U, typename C = List<T>>
		C filtered(const U &f) const;


		iterator begin();
		iterator end();
		const_iterator begin() const;
		const_iterator end() const;
		const_iterator cbegin() const;
		const_iterator cend() const;


		template<typename C>
		bool operator==(const C &l) const;

		template<typename C>
		bool operator!=(const C &l) const;

		template<typename C>
		bool operator<(const C &l) const;


		template<typename C>
		List<T> &operator+=(const C &e);
		template<typename C>
		List<T> &operator<<(const C &e);

	private:
		void append() {
		}

		template<typename C>
		void appendDispatch(const C &t, TrueType) {
			if(head == tail) {
				prependDispatch(t, TrueType());
			} else {
				ListElem *e = new ListElem(t, tail, tail->prev);
				tail->prev = e->prev->next = e;
				lSize++;
			}

		}

		template<typename C>
		void appendDispatch(const C &c, FalseType) {
			for(const auto &e : c) {
				append(e);
			}
		}

		template<typename C>
		void prependDispatch(const C &t, TrueType) {
			ListElem *e = new ListElem(t, head, tail);
			head->prev = e;
			head = e;
			lSize++;
		}

		template<typename C>
		void prependDispatch(const C &c, FalseType) {
			for(const auto &e : c) {
				prepend(e);
			}
		}

		template<typename C>
		iterator insertDispatch(const C &e, iterator t, FalseType) {
			if(t == begin()) {
				prepend(e);
				return begin();
			}
			if(t == end()) {
				append(e);
				return --end();
			}
			ListElem *pr = t.elem->prev;
			ListElem *ne = t.elem;
			ListElem *n = new ListElem(e, ne, pr);
			pr->next = n;
			ne->prev = n;
			lSize++;
			return iterator(n);
		}

		template<typename C>
		iterator insertDispatch(const C &e, iterator t, TrueType) {
			return insert(e.begin(), e.end(), t);
		}


		uint lSize;
		ListElem *tail;
		ListElem *head;

};

}
}

template<typename T>
n::core::List<T> operator+(const T &i, const n::core::List<T> &a) {
	return n::core::List<T>(i, a);
}

template<typename T>
n::core::List<T> operator+(const n::core::List<T> &a, const T &i) {
	return n::core::List<T>(a, i);
}

#include "List_impl.h"

#endif // N_CORE_LIST_H
