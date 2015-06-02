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
#ifndef N_CORE_MAP_H
#define N_CORE_MAP_H

#include "RBTree.h"
#include "Pair.h"

namespace n {
namespace core {

namespace internal {

template<typename T, typename U, typename C>
struct MapOp
{
	bool operator()(const Pair<T, U> &a, const Pair<T, U> &b) const {
		C c;
		return c(a._1, b._1);
	}
};

}

template<typename T, typename U, typename Comp = std::less<T>, typename Eq = std::equal_to<T>>
class Map : public RBTree<Pair<const T, U>, internal::MapOp<const T, U, Comp>, internal::MapOp<const T, U, Eq>>
{
	using MapType = RBTree<Pair<const T, U>, internal::MapOp<const T, U, Comp>, internal::MapOp<const T, U, Eq>>;

	template<typename C>
	struct MapFindOp
	{
		bool operator()(const T &a, const Pair<const T, U> &b) const {
			C c;
			return c(a, b._1);
		}
	};

	typedef MapFindOp<Eq> MapFindEq;
	typedef MapFindOp<Comp> MapFindComp;

	public:
		class iterator : public MapType::iterator
		{
			public:
				iterator(const typename MapType::iterator &it) : MapType::iterator(it) {
				}

				Pair<const T, U> &operator*() {
					return const_cast<Pair<const T, U> &>(MapType::iterator::operator*()); // makes sence
				}

				Pair<const T, U> *operator->() {
					return &operator*();
				}

			private:
				friend class Map;
				iterator(const typename MapType::const_iterator &it) : MapType::iterator(*((typename MapType::iterator *)&it)) {
				}

		};

		typedef typename MapType::const_iterator const_iterator;
		typedef Pair<const T, U> element;

		Map(const MapType &m) : MapType(m) {
		}

		Map() : MapType() {
		}

		iterator insert(const T &t, const U &u) {
			return MapType::insert(element(t, u));
		}

		iterator find(const T &t) {
			return MapType::find(t, MapFindComp(), MapFindEq());
		}

		const_iterator find(const T &t) const {
			return MapType::find(t, MapFindComp(), MapFindEq());
		}

		iterator begin() {
			return iterator(MapType::begin());
		}

		iterator end() {
			return iterator(MapType::end());
		}

		const_iterator begin() const {
			return const_iterator(MapType::begin());
		}

		const_iterator end() const {
			return const_iterator(MapType::end());
		}

		const_iterator cbegin() const {
			return begin();
		}

		const_iterator cend() const {
			return end();
		}

		Map<T, U, Comp, Eq> &operator=(const Map<T, U, Comp, Eq> &o) {
			MapType::operator=(o);
			return *this;
		}

		template<typename C>
		Map<T, U, Comp, Eq> &operator=(const C &o) {
			MapType::operator=(o);
			return *this;
		}

		Map<T, U, Comp, Eq> &operator=(MapType &&o) {
			swap(std::move(o));
			return *this;
		}

		const U &get(const T &t, const U &def) const {
			const_iterator it = find(t);
			return it == end() ? def : (*it)._2;
		}

		const U &get(const T &t) const {
			return get(t, U());
		}

		U &operator[](const T &t) {
			iterator it = find(t);
			if(it == end()) {
				it = insert(t, U());
			}
			return (*it)._2;
		}

		template<typename E>
		Map<T, U, Comp, Eq> operator+(const E &e) const {
			return MapType::operator+(e);
		}

		template<typename E>
		Map<T, U, Comp, Eq> &operator+=(const E &e) {
			MapType::operator+=(e);
			return *this;
		}

		template<typename E>
		Map<T, U, Comp, Eq> &operator<<(const E &e) {
			MapType::operator<<(e);
			return *this;
		}

		template<typename V>
		void map(const V &f) {
			foreach([&](element &e) { e._2 = f(e); });
		}

		template<typename V, typename C = Map<typename std::result_of<V(const element &)>::type, Comp, Eq>>
		C mapped(const V &f) const {
			C a;
			foreach([&](const element &e) { a.insert(f(e)); });
			return a;
		}

		template<typename V, typename C = Map<T, U, Comp, Eq>>
		C filtered(const V &f) const {
			return	C(MapType::filtered(f));
		}
};

} // core
} // n

template<typename T, typename U, typename Comp, typename Eq>
n::core::Map<T, Comp, Eq> operator+(const T &i, n::core::Map<T, Comp, Eq>  &a) {
	n::core::Map<T, Comp, Eq>  b(a);
	b.insert(i);
	return b;
}

template<typename T, typename U, typename Comp, typename Eq>
n::core::Map<T, Comp, Eq> operator+(const n::core::Map<T, Comp, Eq>  &a, const T &i) {
	n::core::Map<T, Comp, Eq>  b(a);
	b.insert(i);
	return b;
}


#endif // N_CORE_MAP_H
