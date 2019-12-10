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
#ifndef Y_CORE_SPAN_H
#define Y_CORE_SPAN_H

#include <y/utils.h>

#include <array>

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

		constexpr MutableSpan() = default;

		constexpr MutableSpan(const MutableSpan&) = default;
		constexpr MutableSpan& operator=(const MutableSpan&) = default;

		constexpr MutableSpan(std::nullptr_t) {
		}

		constexpr MutableSpan(T& t) : _data(&t), _size(1) {
		}

		constexpr MutableSpan(T* data, usize size) : _data(data), _size(size) {
		}

		template<usize N>
		constexpr MutableSpan(T (&arr)[N]) : _data(arr), _size(N) {
		}

		template<usize N>
		constexpr MutableSpan(std::array<T, N>& arr) : _data(arr.data()), _size(N) {
		}

		constexpr MutableSpan(std::initializer_list<T> l) : _data(l.begin()), _size(l.size()) {
		}

		template<typename C, typename = std::enable_if_t<is_compat<data_type<C>>>>
		constexpr MutableSpan(C&& vec) : _data(vec.data()), _size(std::distance(vec.begin(), vec.end())) {
		}

		constexpr usize size() const {
			return _size;
		}

		constexpr bool is_empty() const {
			return !_size;
		}

		constexpr T* data() {
			return _data;
		}

		constexpr const T* data() const {
			return _data;
		}

		constexpr iterator begin() {
			return _data;
		}

		constexpr iterator end() {
			return _data + _size;
		}

		constexpr const_iterator begin() const {
			return _data;
		}

		constexpr const_iterator end() const {
			return _data + _size;
		}

		constexpr const_iterator cbegin() const {
			return _data;
		}

		constexpr const_iterator cend() const {
			return _data + _size;
		}

		constexpr T& operator[](usize i) const {
			y_debug_assert(i < size());
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
