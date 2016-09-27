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
			return _end - _beg;
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


namespace detail {
// from boost
template<typename T>
inline void hash_combine(T& seed, T value) {
	seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<typename T>
static auto has_begin(T*) -> bool_type<!std::is_void<decltype(make_one<T>().begin())>::value>;
template<typename T>
static auto has_begin(...) -> std::false_type;

template<typename T>
static auto has_end(T*) -> bool_type<!std::is_void<decltype(make_one<T>().end())>::value>;
template<typename T>
static auto has_end(...) -> std::false_type;

}

template<typename T>
using is_iterable = bool_type<
		decltype(detail::has_begin<T>(nullptr))::value &&
		decltype(detail::has_end<T>(nullptr))::value
	>;


}
}

namespace std {
	template<typename I>
	struct hash<y::core::Range<I>> {
		typedef y::core::Range<I> argument_type;
		typedef std::size_t result_type;
		result_type operator()(const argument_type& range) const {
			result_type seed = 0;
			for(const auto& i : range) {
				y::core::detail::hash_combine(seed, y::hash(i));
			}
			return seed;
		}
	};

	template<typename T>
	struct hash {
		typedef T argument_type;
		typedef std::size_t result_type;
		template<typename Enable = typename std::enable_if<y::core::is_iterable<T>::value, T>::type>
		result_type operator()(const argument_type& collection) const {
			auto range = y::core::range(collection);
			return y::hash<decltype(range)>(range);
		}
	};
}

#endif // Y_CORE_RANGE_H
