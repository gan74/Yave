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
#ifndef Y_CORE_LAZY_H
#define Y_CORE_LAZY_H

#include <y/utils.h>

namespace y {
namespace core {

template<typename T>
class Lazy : NonCopyable {
	public:
		Lazy(T&& t) : _val(std::forward<T>(t)) {
		}

		Lazy(Lazy&& l) : _val(std::move(l._val)) {
		}

		const T& operator()() const {
			return _val;
		}

	private:
		T _val;
};


template<typename T>
inline auto lazy(T&& t) {
	return Lazy<T>(std::forward<T>(t));
}

}
}

#endif // Y_CORE_LAZY_H
