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

template<typename I, typename F>
class MapIterator
{
	public:
		using Element = typename std::result_of<F(decltype(**make_one<I *>()))>::type;

		MapIterator(const I &beg, const F &f) : it(beg), map(f) {
		}

		MapIterator<I, F> &operator++() {
			++it;
			return *this;
		}

		MapIterator<I, F> &operator--() {
			--it;
			return *this;
		}

		MapIterator<I, F> operator++(int) {
			MapIterator p(*this);
			++it;
			return p;
		}

		MapIterator<I, F> operator--(int) {
			MapIterator p(*this);
			--it;
			return p;
		}

		bool operator!=(const MapIterator<I, F> &i) const {
			return it != i.it || map != i.map;
		}

		bool operator!=(const I &i) const {
			return it != i;
		}

		template<typename T>
		bool operator==(const T &t) const {
			return !operator!=(t);
		}

		Element operator*() {
			return map(*it);
		}


	private:
		I it;
		F map;
};


}
}

#endif // Y_CORE_MAPITERATOR_H
