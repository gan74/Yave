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

namespace details {

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
class Map : public RBTree<Pair<const T, U>, details::MapOp<const T, U, Comp>, details::MapOp<const T, U, Eq>>
{
	using MapType = RBTree<Pair<const T, U>, details::MapOp<const T, U, Comp>, details::MapOp<const T, U, Eq>>;

	template<typename C>
	struct MapFindOp
	{
		bool operator()(const T &a, const Pair<const T, U> &b) const {
			C c;
			return c(a, b._1);
		}
	};

	using MapFindEq = MapFindOp<Eq>;
	using MapFindComp = MapFindOp<Comp>;

	public:
		class iterator : public MapType::iterator
		{
			public:
				iterator(const typename MapType::iterator &it) : MapType::iterator(it) {
				}

				Pair<const T, U> &operator*() const {
					return const_cast<Pair<const T, U> &>(MapType::iterator::operator*()); // makes sence
				}

				Pair<const T, U> *operator->() const {
					return &operator*();
				}

			private:
				friend class Map;
				iterator(const typename MapType::const_iterator &it) : MapType::iterator(*(reinterpret_cast<typename MapType::iterator *>(&it))) {
				}

		};

		using const_iterator = typename MapType::const_iterator;
		using Element = Pair<const T, U>;




		Map();
		Map(MapType &&m);
		Map(const MapType &m);

		template<typename C>
		Map(std::initializer_list<C> l);

		template<typename C, typename CC = typename std::enable_if<Collection<C>::isCollection>::type>
		Map(const C &c);


		iterator insert(const T &t, const U &u);


		const_iterator find(const T &t) const;
		iterator find(const T &t);


		bool exists(const T &t) const;


		iterator begin();
		iterator end();
		const_iterator begin() const;
		const_iterator end() const;
		const_iterator cbegin() const;
		const_iterator cend() const;


		template<typename C>
		Map<T, U, Comp, Eq> &operator=(const C &o);
		Map<T, U, Comp, Eq> &operator=(MapType &&o);
		Map<T, U, Comp, Eq> &operator=(const Map<T, U, Comp, Eq> &o);


		const U &get(const T &t) const;
		const U &get(const T &t, const U &def) const;


		U &operator[](const T &t);


		template<typename E>
		Map<T, U, Comp, Eq> operator+(const E &e) const ;


		template<typename E>
		Map<T, U, Comp, Eq> &operator+=(const E &e);
		template<typename E>
		Map<T, U, Comp, Eq> &operator<<(const E &e);

};

} // core
} // n

#include "Map_impl.h"

#endif // N_CORE_MAP_H
