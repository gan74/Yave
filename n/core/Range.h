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

template<typename T>
class ValueIterator
{
	public:
		ValueIterator(const T &t, bool rev = false) : value(t), reverse(rev) {
		}

		ValueIterator<T> &operator++() {
			inc();
			return *this;
		}

		ValueIterator<T> &operator--() {
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

		bool operator!=(const ValueIterator<T> &t) const {
			return value != t.value;
		}

		bool operator==(const ValueIterator<T> &t) const {
			return value == t.value;
		}

		const T &operator*() const {
			return value;
		}

		const T *operator->() const {
			return &value;
		}

	private:
		T value;
		bool reverse;

		void inc() {
			if(reverse) {
				--value;
			} else {
				++value;
			}
		}

		void dec() {
			if(reverse) {
				++value;
			} else {
				--value;
			}
		}
};


template<typename I>
class Range
{
	N_GEN_TYPE_HAS_METHOD_NRET(CanSub, operator-)
	constexpr static bool hasFastSize = CanSub<I>::value || TypeInfo<I>::isPrimitive;

	public:
		using Element = typename TypeContent<Range<I>>::type;


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
			return sizeDispatch(BoolToType<hasFastSize>());
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

namespace details {

template<typename I, typename RI = ValueIterator<I>>
Range<RI> range(const I &b, const I &e, TrueType) {
	bool r = e < b;
	return Range<RI>(RI(b, r), RI(e, r));
}

template<typename I>
Range<I> range(const I &b, const I &e, FalseType) {
	return Range<I>(b, e);
}

}


template<typename I, typename B = BoolToType<!TypeInfo<I>::isDereferenceable>, typename R = decltype(details::range(makeOne<I>(), makeOne<I>(), makeOne<B>()))>
R range(const I &b, const I &e) {
	return details::range(b, e, B());
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
