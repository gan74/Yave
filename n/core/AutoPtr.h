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

#ifndef N_CORE_AUTOPTR_H
#define N_CORE_AUTOPTR_H

#include "Ptr.h"


namespace n {
namespace core {

template<typename T>
class AutoPtr : public Ptr<T>
{
	using Ptr<T>::ptr;
	static_assert(!std::is_polymorphic<T>::value, "AutoPtr<T> can not store polymorphic objects.");
	public:
		AutoPtr(T *t = 0) : Ptr<T>(std::move(t)) {
		}

		AutoPtr(const AutoPtr<T> &p) : Ptr<T>(0) {
			operator=(p);
		}

		AutoPtr<T> &operator=(const AutoPtr<T> &p) {
			delete ptr;
			ptr = p.ptr ? 0 : new T(*p.ptr);
			return *this;
		}

};

}
}

#endif // N_CORE_AUTOPTR_H

