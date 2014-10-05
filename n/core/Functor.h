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
#include <tuple>
#include "SmartPtr.h"

namespace n {

namespace internal {

template<typename R, uint N>
struct TupleProxy {
	template<typename T, typename... TArgs, typename... Args>
	static R apply(T *t, const std::tuple<TArgs...> &tuple, Args... args) {
		return TupleProxy<R, N - 1>::apply(t, tuple, std::get<N - 1>(tuple), args...);
	}
};
template<typename R>
struct TupleProxy<R, 0> {
	template<typename T, typename... TArgs, typename... Args>
	static R apply(T *t, const std::tuple<TArgs...> &, Args... args) {
		return (*t)(args...);
	}
};

}


namespace core {

template<typename R, typename... Args>
class Functor
{
	class FuncBase
	{
		public:
			FuncBase() {
			}

			FuncBase(const FuncBase &) = delete;

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

			Func(const Func<T> &) = delete;

			virtual R apply(Args... args) override {
				return f(args...);
			}

		private:
			T f;
	};

	class Curry
	{
		public:
			Curry(const Functor<R, Args...> &f, Args... ar) : func(f), args(ar...) {
			}

			Curry(const Functor<R, Args...> &f, std::tuple<Args...> ar) : func(f), args(ar) {
			}

			R operator()() {
				return func(args);
			}

		private:
			Functor<R, Args...> func;
			std::tuple<Args...> args;

	};

	public:
		template<typename T>
		Functor(const T &t) : func(new Func<T>(t)) {
		}

		R operator()(Args... args) {
			return func->apply(args...);
		}

		R operator()(const std::tuple<Args...> &args) {
			return n::internal::TupleProxy<R, sizeof...(Args)>::apply(this, args);
		}

		Functor<R> curry(Args... args) const {
			return Functor<R>(Curry(*this, args...));
		}

		Functor<R> curry(std::tuple<Args...> args) const {
			return Functor<R>(Curry(*this, args));
		}

	private:
		SmartPtr<FuncBase> func;

};

}
}

#endif // FUNCTOR_H
