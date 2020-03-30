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

#include <iterator>

namespace y {

template<typename It, typename Transform>
class TransformIterator : private Transform {
	using iterator_type = It;

	public:
		using value_type = std::remove_reference_t<decltype(std::declval<Transform>()(*std::declval<It>()))>;

		using difference_type = typename It::difference_type;
		using iterator_category = typename It::iterator_category;

		using reference = value_type&;
		using pointer = value_type*;

		TransformIterator(iterator_type it) : _it(it) {
		}

		TransformIterator& operator++() {
			++_it;
			return *this;
		}

		TransformIterator operator++(int) {
			const iterator_type it = _it;
			++_it;
			return TransformIterator(it);
		}

		TransformIterator& operator--() {
			--_it;
			return *this;
		}

		TransformIterator operator--(int) {
			const iterator_type it = _it;
			--_it;
			return TupleMemberIterator(it);
		}

		bool operator==(const TransformIterator& other) const {
			return _it == other._it;
		}

		bool operator!=(const TransformIterator& other) const {
			return _it != other._it;
		}

		auto&& operator*() const {
			return Transform::operator()(*_it);
		}

		auto* operator->() const {
			return Transform::operator()(*_it);
		}


		TransformIterator operator+(usize i) const {
			return TupleMemberIterator(_it + i);
		}

		TransformIterator operator-(usize i) const {
			return TupleMemberIterator(_it - i);
		}

		TransformIterator& operator+=(usize i) const {
			_it += i;
			return *this;
		}

		TransformIterator& operator-=(usize i) const {
			_it -= i;
			return *this;
		}



		template<typename T>
		bool operator==(const T& other) const {
			return _it == other;
		}

		template<typename T>
		bool operator!=(const T& other) const {
			return _it != other;
		}


	private:
		iterator_type _it;
};

namespace detail {
template<usize I>
struct TupleUnpacker {
	template<typename T>
	auto&& operator()(T&& t) const {
		return std::get<I>(y_fwd(t));
	}
};
}

template<usize I, typename It>
using TupleMemberIterator = TransformIterator<It, detail::TupleUnpacker<I>>;


}


#endif // Y_UTILS_ITER_H
