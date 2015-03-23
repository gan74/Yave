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

#ifndef N_CORE_REF
#define N_CORE_REF

#include <n/core/Functor.h>

namespace n {
namespace core {

template<typename T>
class Ref : NonCopyable
{
	public:
		Ref(T &t, const Functor<void()> &f = Nothing()) : value(t), modified(f) {
		}

		Ref(Ref<T> &&r) : value(r.value), modified(r.modified) {
		}

		template<typename U>
		T operator=(const U &t) {
			value = t;
			modified();
			return value;
		}

		operator T() const {
			return value;
		}

	private:
		T &value;
		Functor<void()> modified;
};

}
}

#endif // N_CORE_REF

