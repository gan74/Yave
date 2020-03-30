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

#include "traits.h"

#include <iterator>

namespace y {

struct EndIterator {};

// this might cause ambiguities
template<typename T>
bool operator==(EndIterator, const T& other) {
	return other.at_end();
}

template<typename T>
bool operator==(const T& other, EndIterator) {
	return other.at_end();
}

template<typename T>
bool operator!=(EndIterator, const T& other) {
	return !other.at_end();
}

template<typename T>
bool operator!=(const T& other, EndIterator) {
	return !other.at_end();
}




template<typename It, typename Transform>
class TransformIterator : private Transform {
	using iterator_type = It;
	using iterator_traits = std::iterator_traits<iterator_type>;

	public:
		using value_type = std::remove_reference_t<decltype(std::declval<Transform>()(*std::declval<It>()))>;

		using difference_type = typename iterator_traits::difference_type;
		using iterator_category = typename iterator_traits::iterator_category;

		using reference = value_type&;
		using pointer = value_type*;

		TransformIterator() = default;

		TransformIterator(iterator_type it, const Transform& tr = Transform()) : Transform(tr), _it(it) {
		}

		//template<typename = std::enable_if_t<has_at_end_v<iterator_type>>>
		bool at_end() const {
			static_assert(has_at_end_v<iterator_type>);
			return _it.at_end();
		}

		TransformIterator& operator++() {
			++_it;
			return *this;
		}

		TransformIterator operator++(int) {
			const iterator_type it = _it;
			++_it;
			return TransformIterator(it, *this);
		}

		TransformIterator& operator--() {
			--_it;
			return *this;
		}

		TransformIterator operator--(int) {
			const iterator_type it = _it;
			--_it;
			return TransformIterator(it, *this);
		}

		bool operator==(const TransformIterator& other) const {
			return _it == other._it;
		}

		bool operator!=(const TransformIterator& other) const {
			return _it != other._it;
		}

		decltype(auto) operator*() const {
			return Transform::operator()(*_it);
		}

		auto* operator->() const {
			return &Transform::operator()(*_it);
		}


		TransformIterator operator+(usize i) const {
			return TransformIterator(_it + i, *this);
		}

		TransformIterator operator-(usize i) const {
			return TransformIterator(_it - i, *this);
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

template<typename It, typename Filter, typename End = It>
class FilterIterator : private Filter {
	using iterator_type = It;
	using end_iterator_type = End;
	using iterator_traits = std::iterator_traits<iterator_type>;

	public:
		using value_type = std::remove_reference_t<decltype(*std::declval<It>())>;

		using difference_type = typename iterator_traits::difference_type;
		using iterator_category = std::forward_iterator_tag;

		using reference = value_type&;
		using pointer = value_type*;

		FilterIterator() = default;

		FilterIterator(iterator_type it, end_iterator_type end, const Filter& ft = Filter()) : Filter(ft), _it(it), _end(end) {
			find_next_valid();
		}

		void advance() {
			++_it;
			find_next_valid();
		}

		bool at_end() const {
			return _it == _end;
		}

		FilterIterator& operator++() {
			advance();
			return *this;
		}

		FilterIterator operator++(int) {
			const iterator_type it = _it;
			advance();
			return FilterIterator(it, _end, *this);
		}

		bool operator==(const FilterIterator& other) const {
			return _it == other._it;
		}

		bool operator!=(const FilterIterator& other) const {
			return _it != other._it;
		}

		decltype(auto) operator*() const {
			return *_it;
		}

		auto* operator->() const {
			return *_it;
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
		void find_next_valid() {
			while(!at_end()) {
				if(Filter::operator()(*_it)) {
					break;
				}
				++_it;
			}
		}

		iterator_type _it;
		end_iterator_type _end;
};



namespace detail {
template<usize I>
struct TupleUnpacker {
	template<typename T>
	decltype(auto) operator()(T&& t) const {
		return std::get<I>(y_fwd(t));
	}
};
}

template<usize I, typename It>
using TupleMemberIterator = TransformIterator<It, detail::TupleUnpacker<I>>;


}


#endif // Y_UTILS_ITER_H
