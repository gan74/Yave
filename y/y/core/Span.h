/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#ifndef Y_CORE_SPAN_H
#define Y_CORE_SPAN_H

#include <y/utils.h>

namespace y {
namespace core {

template<typename T>
class MutableSpan {

	template<typename U>
	static constexpr bool is_compat = std::is_constructible_v<T*, U>;

	template<typename C>
	using data_type = decltype(std::declval<C>().data());

	public:
		using value_type = T;
		using iterator = T*;
		using const_iterator = const T*;

		MutableSpan() = default;

		MutableSpan(const MutableSpan&) = default;

		MutableSpan(std::nullptr_t) {
		}

		MutableSpan(T& t) : _data(&t), _size(1) {
		}

		MutableSpan(T* data, usize size) : _data(data), _size(size) {
		}

		template<usize N>
		MutableSpan(T(&arr)[N]) : _data(arr), _size(N) {
		}

		template<usize N>
		MutableSpan(std::array<T, N>& arr) : _data(arr.data()), _size(N) {
		}

		MutableSpan(std::initializer_list<T> l) : _data(l.begin()), _size(l.size()) {
		}

		template<typename C, typename = std::enable_if_t<is_compat<data_type<C>>>>
		MutableSpan(C&& vec) : _data(vec.data()), _size(std::distance(vec.begin(), vec.end())) {
		}

		usize size() const {
			return _size;
		}

		bool is_empty() const {
			return !_size;
		}

		T* data() {
			return _data;
		}

		const T* data() const {
			return _data;
		}

		iterator begin() {
			return _data;
		}

		iterator end() {
			return _data + _size;
		}

		const_iterator begin() const {
			return _data;
		}

		const_iterator end() const {
			return _data + _size;
		}

		const_iterator cbegin() const {
			return _data;
		}

		const_iterator cend() const {
			return _data + _size;
		}

		const T& operator[](usize i) const {
			return _data[i];
		}

	private:
		NotOwner<T*> _data = nullptr;
		usize _size = 0;

};

template<typename T>
using Span = MutableSpan<const T>;
}
}

#endif // Y_CORE_SPAN_H
