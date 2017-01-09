/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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
#ifndef Y_CORE_ARRAYPROXY_H
#define Y_CORE_ARRAYPROXY_H

#include <y/utils.h>

namespace y {
namespace core {

template<typename T>
class ArrayProxy : NonCopyable {

	template<typename U>
	using is_compat = std::is_same<U, const T*>;

	template<typename C>
	using begin_type = decltype(make_one<const C>().begin());

	public:
		using value_type = T;
		using const_iterator = const T*;


		ArrayProxy() = default;

		ArrayProxy(std::nullptr_t) : ArrayProxy() {
		}

		ArrayProxy(const T& t) : _data(&t), _size(1) {
		}

		ArrayProxy(const T* data, usize size) : _data(data), _size(size) {
		}

		template<usize N>
		ArrayProxy(const std::array<T, N>& arr) : _data(arr.data()), _size(N) {
		}

		ArrayProxy(const std::initializer_list<T>& l) : _data(l.begin()), _size(l.size()) {
		}

		template<typename C, typename = std::enable_if_t<is_compat<begin_type<C>>::value>>
		ArrayProxy(const C& vec) : _data(vec.begin()), _size(vec.size()) {
		}

		usize size() const {
			return _size;
		}

		bool is_empty() const {
			return !_size;
		}

		const T* data() const {
			return _data;
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

	private:
		const T* _data = nullptr;
		usize _size = 0;

};

}
}

#endif // Y_CORE_ARRAYPROXY_H
