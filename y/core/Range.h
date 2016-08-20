/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/
#ifndef Y_CORE_RANGE_H
#define Y_CORE_RANGE_H

#include "ReverseIterator.h"
#include "ValueIterator.h"
#include "MapIterator.h"

namespace y {
namespace core {

template<typename I>
class Range {
	public:
		using Element = decltype(**make_one<I *>());

		Range(const I &b, const I &e) : beg(b), en(e) {
		}

		const I &begin() const {
			return beg;
		}

		const I &end() const {
			return en;
		}

		ReverseIterator<I> rbegin() const {
			I i(beg);
			return reverse_iterator(--i);
		}

		ReverseIterator<I> rend() const {
			I i(en);
			return reverse_iterator(--i);
		}

		usize size() const {
			return en - beg;
		}

		Range<ReverseIterator<I>> reverse() const {
			return Range<ReverseIterator<I>>(rend(), rbegin());
		}

		template<typename F>
		Range<MapIterator<I, F>> map(const F &f) const {
			return Range<MapIterator<I, F>>(MapIterator<I, F>(begin(), f), MapIterator<I, F>(end(), f));
		}

		template<typename T>
		void foreach(const T &t) const {
			for(const auto &e : *this) {
				t(e);
			}
		}

		template<typename T>
		void foreach(T &t) const {
			for(const auto &e : *this) {
				t(e);
			}
		}



	private:
		I beg;
		I en;
};

namespace detail {

template<typename I, typename RI = ValueIterator<I>>
Range<RI> range(const I &b, const I &e, std::true_type) {
	bool r = e < b;
	return Range<RI>(RI(b, r), RI(e, r));
}

template<typename I>
Range<I> range(const I &b, const I &e, std::false_type) {
	return Range<I>(b, e);
}

}


template<typename I, typename B = BoolType<!is_dereferenceable<I>::value>, typename R = decltype(detail::range(make_one<I>(), make_one<I>(), make_one<B>()))>
R range(const I &b, const I &e) {
	return detail::range(b, e, B());
}

template<typename C, typename I = decltype(make_one<const C &>().begin())>
Range<I> range(const C &c) {
	return range(c.begin(), c.end());
}

template<typename C, typename I = decltype(make_one<C &>().begin())>
Range<I> range(C &c) {
	return range(c.begin(), c.end());
}

}
}

#endif // Y_CORE_RANGE_H
