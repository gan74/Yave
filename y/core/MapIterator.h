/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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
#ifndef Y_CORE_MAPITERATOR_H
#define Y_CORE_MAPITERATOR_H

#include <y/utils.h>

namespace y {
namespace core {

template<typename Iter, typename Func>
class MapIterator {
	public:
		using Element = typename std::result_of<Func(typename dereference<Iter>::type)>::type;

		MapIterator(const Iter& beg, const Func& f) : _it(beg), _map(f) {
		}

		MapIterator<Iter, Func>& operator++() {
			++_it;
			return *this;
		}

		MapIterator<Iter, Func>& operator--() {
			--_it;
			return *this;
		}

		MapIterator<Iter, Func> operator++(int) {
			MapIterator p(*this);
			++_it;
			return p;
		}

		MapIterator<Iter, Func> operator--(int) {
			MapIterator p(*this);
			--_it;
			return p;
		}

		bool operator!=(const MapIterator<Iter, Func>& i) const {
			return _it != i._it/* || map != i.map*/;
		}

		bool operator!=(const Iter& i) const {
			return _it != i;
		}

		template<typename T>
		bool operator==(const T& t) const {
			return !operator!=(t);
		}

		Element operator*() {
			return _map(*_it);
		}

		auto operator-(const MapIterator& other) const {
			return _it - other._it;
		}

	private:
		Iter _it;
		Func _map;
};


}
}

#endif // Y_CORE_MAPITERATOR_H
