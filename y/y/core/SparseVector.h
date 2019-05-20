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

template<typename Elem, typename Index>
class SparseVector;



namespace detail {
template<bool Const, typename Elem, typename Index>
class SparseVectorPairIterator {

	using vec_type = std::conditional_t<Const, const SparseVector<Elem, Index>, SparseVector<Elem, Index>>;

	public:
		using reference = std::pair<Index, std::conditional_t<Const, const Elem&, Elem&>>;
		using difference_type = usize;
		using iterator_category = std::random_access_iterator_tag;

		reference operator*() const {
			return reference(_vec._dense[_index], _vec._values[_index]);
		}

		reference operator[](usize n) const {
			return *((*this) + n);
		}

		SparseVectorPairIterator& operator++() {
			++_index;
			return *this;
		}

		SparseVectorPairIterator& operator--() {
			--_index;
			return *this;
		}

		SparseVectorPairIterator operator++(int) {
			SparseVectorPairIterator it(*this);
			++_index;
			return it;
		}

		SparseVectorPairIterator operator--(int) {
			SparseVectorPairIterator it(*this);
			--_index;
			return it;
		}

		bool operator==(const SparseVectorPairIterator& other) const {
			return _index == other._index;
		}

		bool operator!=(const SparseVectorPairIterator& other) const {
			return _index != other._index;
		}

		bool operator<(const SparseVectorPairIterator& other) const {
			return _index < other._index;
		}

		bool operator>(const SparseVectorPairIterator& other) const {
			return _index > other._index;
		}

		SparseVectorPairIterator& operator+=(usize n) {
			_index += n;
			return *this;
		}

		SparseVectorPairIterator operator+(usize n) const {
			SparseVectorPairIterator it(*this);
			it += n;
			return it;
		}

		SparseVectorPairIterator& operator-=(usize n) {
			_index -= n;
			return *this;
		}

		SparseVectorPairIterator operator-(usize n) const {
			SparseVectorPairIterator it(*this);
			it -= n;
			return it;
		}

		difference_type operator-(const SparseVectorPairIterator& other) const {
			return _index - other._index;
		}

	private:
		friend class SparseVector<Elem, Index>;

		SparseVectorPairIterator(vec_type& vec, usize index) : _vec(vec), _index(index) {
		}

		vec_type& _vec;
		usize _index = 0;
};
}


template<typename Elem, typename Index>
class SparseVector final {

	struct EmptyVec {
		void emplace_back() {}
		void pop() {}
		void clear() {}
		void swap(EmptyVec&) {}
		void last() {}
	};

	static constexpr bool is_void_v = std::is_void_v<Elem>;
	using non_void = std::conditional_t<is_void_v, int, Elem>;

	static_assert(std::is_integral_v<Index>);

	public:
		using value_type = Elem;
		using size_type = usize;
		using index_type = Index;

		using reference = std::conditional_t<is_void_v, void, non_void&>;
		using const_reference = std::conditional_t<is_void_v, void, const non_void&>;

		using pointer = value_type*;
		using const_pointer = const value_type*;

	private:
		static constexpr usize page_size = 1024;
		static constexpr index_type invalid_index = index_type(-1);

		using page_index_type = u32;
		static constexpr page_index_type page_invalid_index = page_index_type(-1);
		using page_type = std::array<page_index_type, page_size>;

		using value_container = std::conditional_t<is_void_v, EmptyVec, core::Vector<non_void>>;

	public:
		using iterator = typename Vector<non_void>::iterator;
		using const_iterator = typename Vector<non_void>::const_iterator;

		using pair_iterator = detail::SparseVectorPairIterator<false, Elem, Index>;
		using const_pair_iterator = detail::SparseVectorPairIterator<true, Elem, Index>;


		bool has(index_type index) const {
			auto [i, o] = page_index(index);
			return i < _sparse.size() && _sparse[i][o] != page_invalid_index;
		}

		template<typename... Args>
		reference insert(index_type index, Args&&... args) {
			y_debug_assert(!has(index));
			auto [i, o] = page_index(index);
			create_page(i)[o] = page_index_type(_dense.size());
			_dense.emplace_back(index);
			_values.emplace_back(y_fwd(args)...);
			return _values.last();
		}

		void erase(index_type index) {
			y_debug_assert(has(index));
			auto [i, o] = page_index(index);
			page_index_type dense_index = _sparse[i][o];
			page_index_type last_index = page_index_type(_dense.size() - 1);

			std::swap(_dense[dense_index], _dense[last_index]);
			std::swap(_values[dense_index], _values[last_index]);
			_dense.pop();
			_values.pop();

			auto [li, lo] = page_index(index);
			_sparse[li][lo] = dense_index;
			_sparse[i][o] = page_invalid_index;
		}


		reference operator[](index_type index) {
			y_debug_assert(has(index));
			auto [i, o] = page_index(index);
			return _values[_sparse[i][o]];
		}

		const_reference operator[](index_type index) const {
			y_debug_assert(has(index));
			auto [i, o] = page_index(index);
			return _values[_sparse[i][o]];
		}

		pointer try_get(index_type index) {
			auto [i, o] = page_index(index);
			if(i >= _sparse.size()) {
				return nullptr;
			}
			usize pi = _sparse[i][o];
			return pi < _values.size() ? &_values[pi] : nullptr;
		}

		const_pointer try_get(index_type index) const {
			auto [i, o] = page_index(index);
			if(i >= _sparse.size()) {
				return nullptr;
			}
			usize pi = _sparse[i][o];
			return pi < _values.size() ? &_values[pi] : nullptr;
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
			return Range(pair_iterator(*this, 0),
						 pair_iterator(*this, size()));
		}

		auto as_pairs() const {
			return Range(const_pair_iterator(*this, 0),
						 const_pair_iterator(*this, size()));
		}

		MutableSpan<value_type> values() {
			return _values;
		}

		Span<value_type> values() const {
			return _values;
		}

		Span<index_type> indexes() const {
			return _dense;
		}

	private:
		template<bool C, typename E, typename I>
		friend class detail::SparseVectorPairIterator;

		static std::pair<usize, usize> page_index(index_type index) {
			usize i = usize(index) / page_size;
			usize o = usize(index) % page_size;
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

namespace std {
template<bool Const, typename Elem, typename Index>
struct iterator_traits<y::core::detail::SparseVectorPairIterator<Const, Elem, Index>> {
	using iterator_type = typename y::core::detail::SparseVectorPairIterator<Const, Elem, Index>;
	using reference = typename iterator_type::reference;
	using difference_type = typename iterator_type::difference_type;
	using iterator_category = typename iterator_type::iterator_category;
};

}

#endif // Y_CORE_SPARSEVECTOR_H
