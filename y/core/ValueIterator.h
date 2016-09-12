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
#ifndef Y_CORE_VALUEITERATOR_H
#define Y_CORE_VALUEITERATOR_H

#include <y/utils.h>

namespace y {
namespace core {

template<typename T>
class ValueIterator {
	public:
		using Element = T;

		ValueIterator(const T& t, bool rev = false) : value(t), reverse(rev) {
		}

		ValueIterator<T>& operator++() {
			inc();
			return *this;
		}

		ValueIterator<T>& operator--() {
			dec();
			return *this;
		}

		ValueIterator<T> operator++(int) {
			ValueIterator<T> it(*this);
			inc();
			return it;
		}

		ValueIterator<T> operator--(int) {
			ValueIterator<T> it(*this);
			dec();
			return it;
		}

		bool operator!=(const ValueIterator<T>& t) const {
			return value != t.value;
		}

		bool operator==(const ValueIterator<T>& t) const {
			return value == t.value;
		}

		const T& operator*() const {
			return value;
		}

		const T* operator->() const {
			return& value;
		}

	private:
		T value;
		bool reverse;

		void inc() {
			if(reverse) {
				--value;
			} else {
				++value;
			}
		}

		void dec() {
			if(reverse) {
				++value;
			} else {
				--value;
			}
		}
};

}
}

#endif // Y_CORE_VALUEITERATOR_H
