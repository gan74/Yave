/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

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
#ifndef N_CORE_RANGE_H
#define N_CORE_RANGE_H

#include "ReverseIterator.h"

namespace n {
namespace core {

template<typename I>
class Range
{

	N_GEN_TYPE_HAS_METHOD_NRET(CanSub, operator-)

	public:
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
			return reverseIterator(--i);
		}

		ReverseIterator<I> rend() const {
			I i(en);
			return reverseIterator(--i);
		}

		Range<ReverseIterator<I>> reverse() const {
			return Range<ReverseIterator<I>>(rend(), rbegin());
		}

		uint size() const {
			return sizeDispatch(BoolToType<CanSub<I>::value || TypeInfo<I>::isPrimitive>());
		}


	private:
		uint sizeDispatch(TrueType) const {
			return en - beg;
		}

		uint sizeDispatch(FalseType) const {
			uint s = 0;
			for(I i = beg; i != en; ++i, s++);
			return s;
		}

		I beg;
		I en;
};


template<typename I>
Range<I> range(const I &b, const I &e) {
	return Range<I>(b, e);
}

template<typename C, typename I = decltype(makeOne<const C &>().begin())>
Range<I> range(const C &c) {
	return range(c.begin(), c.end());
}

template<typename C, typename I = decltype(makeOne<C &>().begin())>
Range<I> range(C &c) {
	return range(c.begin(), c.end());
}




}
}

#endif // N_CORE_RANGE_H
