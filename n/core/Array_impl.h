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

template<typename T, typename RP>
Array<T,RP>::Array() : RP(), data(0), dataEnd(0), allocEnd(0) {
}

template<typename T, typename RP>
Array<T, RP>::~Array() {
	makeEmpty();
	free(data);
}

template<typename T, typename RP>
Array<T, RP>::Array(const Array<T, RP> &a) : Array<T, RP>(a.size()) {
	copy(data, a.data, a.size());
	dataEnd = data + a.size();
}

template<typename T, typename RP>
Array<T, RP>::Array(Array<T, RP> &&a) : Array<T, RP>() {
	swap(std::move(a));
}

template<typename T, typename RP>
template<typename C, typename R>
Array<T, RP>::Array(const Array<C, R> &a) : Array<T, RP>(a.size()) {
	append(a);
}

template<typename T, typename RP>
template<typename C>
Array<T, RP>::Array(std::initializer_list<C> l) : Array<T, RP>((l.size())) {
	for(auto x : l) {
		append(x);
	}
}

template<typename T, typename RP>
Array<T,RP>::Array(uint s) : Array() {
	setCapacityUnsafe(0, s);
}



}
}
