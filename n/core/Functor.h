/*******************************
Copyright (C) 2013-2014 gr�goire ANGERAND

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

#ifndef N_CORE_FUNCTOR_H
#define N_CORE_FUNCTOR_H

#include <n/Types.h>
#include "SmartPtr.h"

namespace n {
namespace core {

template<typename R, typename... Args>
class Functor
{
	class FuncBase
	{
		public:
			virtual ~FuncBase() {
			}

			virtual R apply(Args...) = 0;
	};

	template<typename T>
	class Func : public FuncBase
	{
		public:
			Func(const T &t) : f(t) {
			}

			R apply(Args... args) override {
				return f(args...);
			}

		private:
			T f;
	};

	public:
		template<typename T>
		Functor(const T &t) : func(new Func<T>(t)) {
		}

		R operator()(Args... args) {
			return func->apply(args...);
		}

	private:
		SmartPtr<FuncBase> func;

};

}
}

#endif // FUNCTOR_H
