/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

This code is licensed under the MIT License (MIT).

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
#ifndef Y_CORE_REVERSEITERATOR_H
#define Y_CORE_REVERSEITERATOR_H

#include <y/utils.h>

namespace y {
namespace core {

template<typename Iter>
class ReverseIterator {
	public:
		using Element = typename dereference<Iter>::type;

		explicit ReverseIterator(const Iter& beg) : _it(beg) {
		}

		ReverseIterator<Iter>& operator++() {
			--_it;
			return *this;
		}

		ReverseIterator<Iter>& operator--() {
			++_it;
			return *this;
		}

		ReverseIterator<Iter> operator++(int) {
			ReverseIterator p(*this);
			--_it;
			return p;
		}

		ReverseIterator<Iter> operator--(int) {
			ReverseIterator p(*this);
			++_it;
			return p;
		}

		bool operator!=(const ReverseIterator<Iter>& i) const {
			return _it != i._it;
		}

		bool operator!=(const Iter& i) const {
			return _it != i;
		}

		template<typename T>
		bool operator==(const T& t) const {
			return !operator!=(t);
		}

		const Element& operator*() const {
			return *_it;
		}

		Element& operator*() {
			return *_it;
		}

		auto operator-(const ReverseIterator& other) const {
			return other._it - _it;
		}

	private:
		Iter _it;
};

template<typename Iter>
inline ReverseIterator<Iter> reverse_iterator(const Iter& i) {
	return ReverseIterator<Iter>(i);
}

}
}

#endif // Y_CORE_REVERSEITERATOR_H
