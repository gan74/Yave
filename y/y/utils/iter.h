/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef Y_UTILS_ITER_H
#define Y_UTILS_ITER_H

#include "types.h"

namespace y {

template<usize I, typename It>
class TupleMemberIterator {
	using iterator_type = It;

	public:
		TupleMemberIterator(iterator_type it) : _it(it) {
		}

		TupleMemberIterator& operator++() {
			++_it;
			return *this;
		}

		TupleMemberIterator operator++(int) {
			const iterator_type it = _it;
			++_it;
			return TupleMemberIterator(it);
		}

		TupleMemberIterator& operator--() {
			--_it;
			return *this;
		}

		TupleMemberIterator operator--(int) {
			const iterator_type it = _it;
			--_it;
			return TupleMemberIterator(it);
		}

		bool operator==(const TupleMemberIterator& other) const {
			return _it == other._it;
		}

		bool operator!=(const TupleMemberIterator& other) const {
			return _it != other._it;
		}

		template<typename T>
		bool operator==(const T& other) const {
			return _it == other;
		}

		template<typename T>
		bool operator!=(const T& other) const {
			return _it != other;
		}

		auto&& operator*() const {
			return std::get<I>(*_it);
		}

		auto* operator->() const {
			return &std::get<I>(*_it);
		}


		TupleMemberIterator operator+(usize i) const {
			return TupleMemberIterator(_it + i);
		}

		TupleMemberIterator operator-(usize i) const {
			return TupleMemberIterator(_it - i);
		}

		TupleMemberIterator& operator+=(usize i) const {
			_it += i;
			return *this;
		}

		TupleMemberIterator& operator-=(usize i) const {
			_it -= i;
			return *this;
		}

	private:
		iterator_type _it;
};


}


#endif // Y_UTILS_ITER_H
