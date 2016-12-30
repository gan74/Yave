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
#ifndef Y_CORE_RANGE_H
#define Y_CORE_RANGE_H

#include <y/utils.h>
#include "ReverseIterator.h"
#include "ValueIterator.h"
#include "MapIterator.h"
#include "FilterIterator.h"

namespace y {
namespace core {

template<typename Iter>
class Range {
	public:
		using Return = typename dereference<Iter>::type;
		using Element = typename std::decay<Return>::type;

		Range(const Iter& b, const Iter& e) : _beg(b), _end(e) {
		}

		const Iter& begin() const {
			return _beg;
		}

		const Iter& end() const {
			return _end;
		}

		ReverseIterator<Iter> rbegin() const {
			Iter i(_beg);
			return reverse_iterator(--i);
		}

		ReverseIterator<Iter> rend() const {
			Iter i(_end);
			return reverse_iterator(--i);
		}

		usize size() const {
			return std::distance(_beg, _end);
		}

		Range<ReverseIterator<Iter>> reverse() const {
			return Range<ReverseIterator<Iter>>(rend(), rbegin());
		}

		template<typename F>
		Range<MapIterator<Iter, F>> map(const F& f) const {
			return Range<MapIterator<Iter, F>>(MapIterator<Iter, F>(begin(), f), MapIterator<Iter, F>(end(), f));
		}

		template<typename F>
		Range<FilterIterator<Iter, F>> filter(const F& f) const {
			return Range<FilterIterator<Iter, F>>(FilterIterator<Iter, F>(begin(), f), FilterIterator<Iter, F>(end(), f));
		}

		template<typename T>
		auto find(const T& t) {
			return find_dsp(t, is_comparable<Element, T>());
		}

		template<template<typename...> typename Coll>
		auto collect() const {
			Coll<Element> c;
			collect(c);
			return c;
		}

		template<typename Stream>
		auto collect(Stream& str) const {
			for(const auto& e : *this) {
				str << e;
			}
		}

		template<typename T>
		void foreach(T&& t) const {
			for(const auto& e : *this) {
				t(e);
			}
		}

		template<typename T>
		bool contains(const T& t) const {
			for(const auto& e : *this) {
				if(e == t) {
					return true;
				}
			}
			return false;
		}

	private:
		template<typename T>
		Iter find_dsp(const T& t, std::true_type) {
			for(auto it = _beg; it != _end; ++it) {
				if(*it == t) {
					return it;
				}
			}
			return _end;
		}

		template<typename F>
		Iter find_dsp(const F& func, std::false_type) {
			for(auto it = _beg; it != _end; ++it) {
				if(func(*it)) {
					return it;
				}
			}
			return _end;
		}

		Iter _beg;
		Iter _end;
};

namespace detail {

template<typename Iter>
inline auto range(const Iter& b, const Iter& e, std::true_type) {
	bool r = e < b;
	using RI = ValueIterator<Iter>;
	return Range<RI>(RI(b, r), RI(e, r));
}

template<typename Iter>
inline auto range(const Iter& b, const Iter& e, std::false_type) {
	return Range<Iter>(b, e);
}

}


template<typename Iter>
inline auto range(const Iter& b, const Iter& e) {
	return detail::range(b, e, bool_type<!is_dereferenceable<Iter>::value>());
}

template<typename Coll>
inline auto range(Coll&& c) {
	return range(c.begin(), c.end());
}

}
}

#endif // Y_CORE_RANGE_H
