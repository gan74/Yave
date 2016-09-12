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
#ifndef Y_CORE_FILTERITERATOR_H
#define Y_CORE_FILTERITERATOR_H

#include <y/utils.h>

namespace y {
namespace core {

template<typename Iter, typename Func>
class FilterIterator {
	public:
		using Element = typename dereference<Iter>::type;

		FilterIterator(const Iter& beg, const Func& f) : _it(beg), _filter(f) {
		}

		FilterIterator<Iter, Func>& operator++() {
			while(!_filter(*(++_it)));
			return *this;
		}

		FilterIterator<Iter, Func>& operator--() {
			while(!_filter(*(--_it)));
			return *this;
		}

		FilterIterator<Iter, Func> operator++(int) {
			FilterIterator p(*this);
			while(!_filter(*(++_it)));
			return p;
		}

		FilterIterator<Iter, Func> operator--(int) {
			FilterIterator p(*this);
			while(!_filter(*(--_it)));
			return p;
		}

		bool operator!=(const FilterIterator<Iter, Func>& i) const {
			return _it != i._it || _filter != i._filter;
		}

		bool operator!=(const Iter& i) const {
			return _it != i;
		}

		template<typename T>
		bool operator==(const T& t) const {
			return !operator!=(t);
		}

		Element operator*() {
			return *_it;
		}


	private:
		Iter _it;
		Func _filter;
};


}
}

#endif // Y_CORE_FILTERITERATOR_H
