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
#ifndef Y_CORE_MAPITERATOR_H
#define Y_CORE_MAPITERATOR_H

#include <y/utils.h>

namespace y {
namespace core {

template<typename Iter, typename Func>
class MapIterator {
	public:
		using Element = typename std::result_of<Func(typename dereference<Iter>::type)>::type;

		MapIterator(const Iter& beg, const Func& f) : _it(beg), _map(f) {
		}

		MapIterator<Iter, Func>& operator++() {
			++_it;
			return *this;
		}

		MapIterator<Iter, Func>& operator--() {
			--_it;
			return *this;
		}

		MapIterator<Iter, Func> operator++(int) {
			MapIterator p(*this);
			++_it;
			return p;
		}

		MapIterator<Iter, Func> operator--(int) {
			MapIterator p(*this);
			--_it;
			return p;
		}

		bool operator!=(const MapIterator<Iter, Func>& i) const {
			return _it != i._it/* || map != i.map*/;
		}

		bool operator!=(const Iter& i) const {
			return _it != i;
		}

		template<typename T>
		bool operator==(const T& t) const {
			return !operator!=(t);
		}

		Element operator*() {
			return _map(*_it);
		}

		auto operator-(const MapIterator& other) const {
			return _it - other._it;
		}

	private:
		Iter _it;
		Func _map;
};


}
}

#endif // Y_CORE_MAPITERATOR_H
