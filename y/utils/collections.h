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
#ifndef Y_UTILS_COLLECTIONS_H
#define Y_UTILS_COLLECTIONS_H

#include <iterator>

namespace y {

template<typename B, typename E = B>
class Range {
	public:
		using iterator_traits = std::iterator_traits<B>;

		Range(const B& b, const E& e) : _begin(b), _end(e) {
		}

		B begin() const {
			return _begin;
		}

		E end() const {
			return _end;
		}

		B begin() {
			return _begin;
		}

		E end() {
			return _end;
		}

		bool operator==(const Range& other) const {
			return _begin == other._begin && _end == other._end;
		}

		bool operator!=(const Range& other) const {
			return !operator==(other);
		}

	private:
		B _begin;
		E _end;
};

template<typename B, typename E>
auto range(const B& b, const E& e) {
	return Range<B, E>(b, e);
}






namespace detail {
template<typename T>
struct has_reserve {
	template<typename C> static auto test(void*) -> decltype(std::declval<C>().reserve(0), std::true_type{});
	template<typename> static std::false_type test(...);

	using type = decltype(test<T>(nullptr));
};

template<typename T>
void try_reserve(T& t, usize size, std::true_type) {
	t.reserve(size);
}

template<typename T>
void try_reserve(T&, usize, std::false_type) {
}
}

template<typename T>
void try_reserve(T& t, usize size) {
	detail::try_reserve(t, size, typename detail::has_reserve<T>::type());
}


}

#endif // Y_UTILS_COLLECTIONS_H
