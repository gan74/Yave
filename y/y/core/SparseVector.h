/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#ifndef Y_CORE_SPARSEVECTOR_H
#define Y_CORE_SPARSEVECTOR_H

#include "Vector.h"
#include "Range.h"


namespace y {
namespace core {

namespace detail {
class SparseIndexIterator;
}

template<typename Elem>
class SparseVector {

	struct EmptyVec {
		void emplace_back() {}
		void pop() {}
		void clear() {}
		void swap(EmptyVec&) {}
		void last() {}
	};

	static constexpr bool is_void_v = std::is_void_v<Elem>;
	using non_void = std::conditional_t<is_void_v, int, Elem>;

	public:
		using value_type = Elem;
		using size_type = usize;
		using index_type = usize;

		using reference = std::conditional_t<is_void_v, void, non_void&>;
		using const_reference = std::conditional_t<is_void_v, void, const non_void&>;

		using pointer = value_type*;
		using const_pointer = const value_type*;

	private:
		friend class detail::SparseIndexIterator;

		static constexpr usize page_size = 256;
		static constexpr index_type invalid_index = index_type(-1);
		using page_type = std::array<index_type, page_size>;

		using value_container = std::conditional_t<is_void_v, EmptyVec, core::Vector<non_void>>;

		template<bool Const>
		class PairIterator {
			public:
				using value_type = std::pair<Elem, index_type>;
				using reference = std::pair<index_type, std::conditional_t<Const, const Elem&, Elem&>>;
				using difference_type = usize;
				using iterator_category = std::random_access_iterator_tag;

				reference operator*() const {
					return {_vec._dense[_index], _vec._values[_index]};
				}

				reference operator[](usize n) const {
					return *((*this) + n);
				}

				PairIterator& operator++() {
					++_index;
					return *this;
				}

				PairIterator& operator--() {
					--_index;
					return *this;
				}

				PairIterator operator++(int) {
					PairIterator it(*this);
					++_index;
					return it;
				}

				PairIterator operator--(int) {
					PairIterator it(*this);
					--_index;
					return it;
				}

				bool operator==(const PairIterator& other) const {
					return _index == other._index;
				}

				bool operator!=(const PairIterator& other) const {
					return _index != other._index;
				}

				bool operator<(const PairIterator& other) const {
					return _index < other._index;
				}

				bool operator>(const PairIterator& other) const {
					return _index > other._index;
				}

				PairIterator& operator+=(usize n) {
					_index += n;
					return *this;
				}

				PairIterator operator+(usize n) const {
					PairIterator it(*this);
					it += n;
					return it;
				}

				PairIterator& operator-=(usize n) {
					_index -= n;
					return *this;
				}

				PairIterator operator-(usize n) const {
					PairIterator it(*this);
					it -= n;
					return it;
				}

				difference_type operator-(const PairIterator& other) const {
					return _index - other._index;
				}

			private:
				friend class SparseVector;

				PairIterator(SparseVector& vec, index_type index) : _vec(vec), _index(index) {
				}

				SparseVector& _vec;
				index_type _index = 0;
		};

	public:
		using iterator = typename Vector<non_void>::iterator;
		using const_iterator = typename Vector<non_void>::const_iterator;


		bool has(index_type index) const {
			auto [i, o] = page_index(index);
			return i < _sparse.size() && _sparse[i][o] != invalid_index;
		}

		template<typename... Args>
		reference insert(index_type index, Args&&... args) {
			auto [i, o] = page_index(index);
			create_page(i)[o] = _dense.size();
			_dense.emplace_back(index);
			_values.emplace_back(y_fwd(args)...);
			return _values.last();
		}

		void erase(index_type index) {
			auto [i, o] = page_index(index);
			index_type dense_index = _sparse[i][o];
			index_type last_index = _dense.size() - 1;

			std::swap(_dense[dense_index], _dense[last_index]);
			std::swap(_values[dense_index], _values[last_index]);
			_dense.pop();
			_values.pop();

			auto [li, lo] = page_index(index);
			_sparse[li][lo] = dense_index;
			_sparse[i][o] = invalid_index;
		}


		reference operator[](index_type index) {
			auto [i, o] = page_index(index);
			return _values[_sparse[i][o]];
		}

		const_reference operator[](usize index) const {
			auto [i, o] = page_index(index);
			return _values[_sparse[i][o]];
		}


		void set_min_capacity(usize cap) {
			_values.set_min_capacity(cap);
			_dense.set_min_capacity(cap);
		}

		void clear() {
			_values.clear();
			_dense.clear();
			_sparse.clear();
		}


		void swap(SparseVector& v) {
			if(&v != this) {
				_values.swap(v._values);
				_dense.swap(v._dense);
				_sparse.swap(v._sparse);
			}
		}

		usize size() const {
			return _dense.size();
		}

		bool is_empty() const {
			return _dense.is_empty();
		}

		const_iterator begin() const {
			return _values.begin();
		}

		const_iterator end() const {
			return _values.end();
		}

		const_iterator cbegin() const {
			return _values.begin();
		}

		const_iterator cend() const {
			return _values.end();
		}

		iterator begin() {
			return _values.begin();
		}

		iterator end() {
			return _values.end();
		}

		pointer data() {
			return _values.data();
		}

		const_pointer data() const {
			return _values.data();
		}

		auto as_pairs() {
			return Range(PairIterator<false>(*this, 0),
						 PairIterator<false>(*this, size()));
		}

		auto as_pairs() const {
			return Range(PairIterator<true>(*this, 0),
						 PairIterator<true>(*this, size()));
		}

		ArrayView<index_type> indexes() const {
			return _dense;
		}

	private:
		static std::pair<usize, usize> page_index(index_type index) {
			usize i = index / page_size;
			usize o = index % page_size;
			return {i, o};
		}

		page_type& create_page(usize page_i) {
			while(page_i >= _sparse.size()) {
				_sparse.emplace_back();
				auto& page = _sparse.last();
				std::fill(page.begin(), page.end(), invalid_index);
			}
			return _sparse[page_i];
		}

		value_container _values;
		Vector<index_type> _dense;
		Vector<page_type> _sparse;
};

}
}



#endif // Y_CORE_SPARSEVECTOR_H
