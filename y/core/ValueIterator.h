/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#ifndef Y_CORE_VALUEITERATOR_H
#define Y_CORE_VALUEITERATOR_H

#include <y/utils.h>

namespace y {
namespace core {

template<typename T>
class ValueIterator {
	public:
		using Element = T;

		ValueIterator(const T& t, bool rev = false) : _value(t), _reverse(rev) {
		}

		ValueIterator<T>& operator++() {
			inc();
			return *this;
		}

		ValueIterator<T>& operator--() {
			dec();
			return *this;
		}

		ValueIterator<T> operator++(int) {
			ValueIterator<T> it(*this);
			inc();
			return it;
		}

		ValueIterator<T> operator--(int) {
			ValueIterator<T> it(*this);
			dec();
			return it;
		}

		bool operator!=(const ValueIterator<T>& t) const {
			return _value != t._value;
		}

		bool operator==(const ValueIterator<T>& t) const {
			return _value == t._value;
		}

		const T& operator*() const {
			return _value;
		}

		const T* operator->() const {
			return& _value;
		}

		auto operator-(const ValueIterator& other) const {
			return _reverse ? other._value - _value : _value - other._value;
		}

	private:
		T _value;
		bool _reverse;

		void inc() {
			if(_reverse) {
				--_value;
			} else {
				++_value;
			}
		}

		void dec() {
			if(_reverse) {
				++_value;
			} else {
				--_value;
			}
		}
};

}
}

#endif // Y_CORE_VALUEITERATOR_H
