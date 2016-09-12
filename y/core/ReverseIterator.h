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
#ifndef Y_CORE_REVERSEITERATOR_H
#define Y_CORE_REVERSEITERATOR_H

#include <y/utils.h>

namespace y {
namespace core {

template<typename Iter>
class ReverseIterator {
	public:
		using Element = typename dereference<Iter>::type;

		explicit ReverseIterator(const Iter& beg) : _it(beg) {
		}

		ReverseIterator<Iter>& operator++() {
			--_it;
			return *this;
		}

		ReverseIterator<Iter>& operator--() {
			++_it;
			return *this;
		}

		ReverseIterator<Iter> operator++(int) {
			ReverseIterator p(*this);
			--_it;
			return p;
		}

		ReverseIterator<Iter> operator--(int) {
			ReverseIterator p(*this);
			++_it;
			return p;
		}

		bool operator!=(const ReverseIterator<Iter>& i) const {
			return _it != i._it;
		}

		bool operator!=(const Iter& i) const {
			return _it != i;
		}

		template<typename T>
		bool operator==(const T& t) const {
			return !operator!=(t);
		}

		const Element& operator*() const {
			return *_it;
		}

		Element& operator*() {
			return *_it;
		}


	private:
		Iter _it;
};

template<typename Iter>
inline ReverseIterator<Iter> reverse_iterator(const Iter& i) {
	return ReverseIterator<Iter>(i);
}

}
}

#endif // Y_CORE_REVERSEITERATOR_H
