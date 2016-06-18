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
#ifndef N_CORE_REVERSEITERATOR_H
#define N_CORE_REVERSEITERATOR_H

#include <n/types.h>

namespace n {
namespace core {

template<typename I>
class ReverseIterator
{
	public:
		ReverseIterator(const I &beg) : it(beg) {
		}


		ReverseIterator<I> &operator++() {
			--it;
			return *this;
		}

		ReverseIterator<I> &operator--() {
			++it;
			return *this;
		}

		ReverseIterator<I> operator++(int) {
			ReverseIterator p(*this);
			--it;
			return p;
		}

		ReverseIterator<I> operator--(int) {
			ReverseIterator p(*this);
			++it;
			return it;
		}

		bool operator!=(const ReverseIterator<I> &i) const {
			return it != i.it;
		}

		bool operator==(const ReverseIterator<I> &i) const {
			return it == i.it;
		}

		bool operator!=(const I &i) const {
			return it != i;
		}

		bool operator==(const I &i) const {
			return it == i;
		}

		typename TypeContent<I>::type operator*() {
			return *it;
		}


	private:
		I it;
};

template<typename I>
ReverseIterator<I> reverseIterator(const I &i) {
	return ReverseIterator<I>(i);
}

}
}

#endif // N_CORE_REVERSEITERATOR_H
