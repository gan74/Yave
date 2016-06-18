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



template<typename T, typename U, typename Comp, typename Eq>
Map<T, U, Comp, Eq>::Map() : MapType() {
}

template<typename T, typename U, typename Comp, typename Eq>
Map<T, U, Comp, Eq>::Map(MapType &&m) : Map() {
	this->swap(m);
}

template<typename T, typename U, typename Comp, typename Eq>
Map<T, U, Comp, Eq>::Map(const MapType &m) : MapType(m) {
}

template<typename T, typename U, typename Comp, typename Eq>
template<typename C>
Map<T, U, Comp, Eq>::Map(std::initializer_list<C> l) : MapType(l) {
}

template<typename T, typename U, typename Comp, typename Eq>
template<typename C, typename CC>
Map<T, U, Comp, Eq>::Map(const C &c) : MapType(c) {
}







template<typename T, typename U, typename Comp, typename Eq>
typename Map<T, U, Comp, Eq>::iterator Map<T, U, Comp, Eq>::insert(const T &t, const U &u) {
	return MapType::insert(Element(t, u));
}








template<typename T, typename U, typename Comp, typename Eq>
typename Map<T, U, Comp, Eq>::iterator Map<T, U, Comp, Eq>::find(const T &t) {
	return MapType::find(t, MapFindComp(), MapFindEq());
}

template<typename T, typename U, typename Comp, typename Eq>
typename Map<T, U, Comp, Eq>::const_iterator Map<T, U, Comp, Eq>::find(const T &t) const {
	return MapType::find(t, MapFindComp(), MapFindEq());
}









template<typename T, typename U, typename Comp, typename Eq>
bool Map<T, U, Comp, Eq>::exists(const T &t) const {
	return find(t) != end();
}










template<typename T, typename U, typename Comp, typename Eq>
typename Map<T, U, Comp, Eq>::iterator Map<T, U, Comp, Eq>::begin() {
	return iterator(MapType::begin());
}

template<typename T, typename U, typename Comp, typename Eq>
typename Map<T, U, Comp, Eq>::iterator Map<T, U, Comp, Eq>::end() {
	return iterator(MapType::end());
}

template<typename T, typename U, typename Comp, typename Eq>
typename Map<T, U, Comp, Eq>::const_iterator Map<T, U, Comp, Eq>::begin() const {
	return const_iterator(MapType::begin());
}

template<typename T, typename U, typename Comp, typename Eq>
typename Map<T, U, Comp, Eq>::const_iterator Map<T, U, Comp, Eq>::end() const {
	return const_iterator(MapType::end());
}

template<typename T, typename U, typename Comp, typename Eq>
typename Map<T, U, Comp, Eq>::const_iterator Map<T, U, Comp, Eq>::cbegin() const {
	return begin();
}

template<typename T, typename U, typename Comp, typename Eq>
typename Map<T, U, Comp, Eq>::const_iterator Map<T, U, Comp, Eq>::cend() const {
	return end();
}








template<typename T, typename U, typename Comp, typename Eq>
Map<T, U, Comp, Eq> &Map<T, U, Comp, Eq>::operator=(const Map<T, U, Comp, Eq> &o) {
	MapType::operator=(o);
	return *this;
}

template<typename T, typename U, typename Comp, typename Eq>
template<typename C>
Map<T, U, Comp, Eq> &Map<T, U, Comp, Eq>::operator=(const C &o) {
	MapType::operator=(o);
	return *this;
}

template<typename T, typename U, typename Comp, typename Eq>
Map<T, U, Comp, Eq> &Map<T, U, Comp, Eq>::operator=(MapType &&o) {
	swap(std::move(o));
	return *this;
}









template<typename T, typename U, typename Comp, typename Eq>
const U &Map<T, U, Comp, Eq>::get(const T &t, const U &def) const {
	const_iterator it = find(t);
	return it == end() ? def : (*it)._2;
}

template<typename T, typename U, typename Comp, typename Eq>
const U &Map<T, U, Comp, Eq>::get(const T &t) const {
	return get(t, U());
}








template<typename T, typename U, typename Comp, typename Eq>
U &Map<T, U, Comp, Eq>::operator[](const T &t) {
	iterator it = find(t);
	if(it == end()) {
		it = insert(t, U());
	}
	return (*it)._2;
}










template<typename T, typename U, typename Comp, typename Eq>
template<typename E>
Map<T, U, Comp, Eq> Map<T, U, Comp, Eq>::operator+(const E &e) const {
	return MapType::operator+(e);
}










template<typename T, typename U, typename Comp, typename Eq>
template<typename E>
Map<T, U, Comp, Eq> &Map<T, U, Comp, Eq>::operator+=(const E &e) {
	MapType::operator+=(e);
	return *this;
}

template<typename T, typename U, typename Comp, typename Eq>
template<typename E>
Map<T, U, Comp, Eq> &Map<T, U, Comp, Eq>::operator<<(const E &e) {
	MapType::operator<<(e);
	return *this;
}








}
}
