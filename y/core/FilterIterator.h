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
#ifndef Y_CORE_FILTERITERATOR_H
#define Y_CORE_FILTERITERATOR_H

#include <y/utils.h>

namespace y {
namespace core {

template<typename Iter, typename Func>
class FilterIterator {
	public:
		using Element = typename dereference<Iter>::type;

		FilterIterator(const Iter& beg, const Func& f) : _it(beg), _filter(f) {
		}

		FilterIterator<Iter, Func>& operator++() {
			while(!_filter(*(++_it)));
			return *this;
		}

		FilterIterator<Iter, Func>& operator--() {
			while(!_filter(*(--_it)));
			return *this;
		}

		FilterIterator<Iter, Func> operator++(int) {
			FilterIterator p(*this);
			while(!_filter(*(++_it)));
			return p;
		}

		FilterIterator<Iter, Func> operator--(int) {
			FilterIterator p(*this);
			while(!_filter(*(--_it)));
			return p;
		}

		bool operator!=(const FilterIterator<Iter, Func>& i) const {
			return _it != i._it || _filter != i._filter;
		}

		bool operator!=(const Iter& i) const {
			return _it != i;
		}

		template<typename T>
		bool operator==(const T& t) const {
			return !operator!=(t);
		}

		Element operator*() {
			return *_it;
		}

		auto operator-(const FilterIterator& other) const {
			return _it - other._it;
		}

	private:
		Iter _it;
		Func _filter;
};


}
}

#endif // Y_CORE_FILTERITERATOR_H
