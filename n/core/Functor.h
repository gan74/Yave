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

#ifndef N_CORE_FUNCTOR_H
#define N_CORE_FUNCTOR_H

#include "SmartPtr.h"
#include <tuple>
#include "Map.h"

namespace n {

namespace details {

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
class Functor {};

template<typename R, typename... Args>
class Functor<R(Args...)>
{
	class FuncBase : NonCopyable
	{
		public:
			FuncBase() {
			}

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

			virtual ~Func() {
			}

			virtual R apply(Args... args) override {
				return f(args...);
			}

		private:
			T f;
	};

	class FuncPtr : public FuncBase
	{
		public:
			FuncPtr(R (*t)(Args...)) : f(t) {
			}

			virtual ~FuncPtr() {
			}

			virtual R apply(Args... args) override {
				return f(args...);
			}

		private:
			R (*f)(Args...);
	};

	template<typename T>
	FuncBase *voidify(const T &t, FalseType) {
		return new Func<T>(t);
	}

	template<typename T>
	FuncBase *voidify(const T &t, TrueType) {
		return voidify([=] (Args... args) -> void {
			t(args...);
		}, FalseType());
	}

	class Curry
	{
		public:
			Curry(const Functor<R(Args...)> &f, Args... ar) : func(f), args(ar...) {
			}

			Curry(const Functor<R(Args...)> &f, std::tuple<Args...> ar) : func(f), args(ar) {
			}

			R operator()() {
				return func(args);
			}

			R operator()() const {
				return func(args);
			}

		private:
			Functor<R(Args...)> func;
			std::tuple<Args...> args;

	};

	public:
		using ReturnType = R;

		template<typename T>
		Functor(const T &t) : func(voidify(t, BoolToType<std::is_void<R>::value>())) {
		}

		Functor(R (*f)(Args...)) : func(new FuncPtr(f)) {
		}

		Functor() : Functor<R(Args...)>(Nothing()) {
		}

		R operator()(Args... args) {
			return func->apply(args...);
		}

		R operator()(const std::tuple<Args...> &args) {
			return n::details::TupleProxy<R, sizeof...(Args)>::apply(this, args);
		}

		R operator()(Args... args) const {
			return func->apply(args...);
		}

		R operator()(const std::tuple<Args...> &args) const {
			return n::details::TupleProxy<R, sizeof...(Args)>::apply(this, args);
		}

		Functor<R()> curried(Args... args) const {
			return Functor<R()>(Curry(*this, args...));
		}

		Functor<R()> curried(std::tuple<Args...> args) const {
			return Functor<R()>(Curry(*this, args));
		}

		Functor<R(Args...)> memo() const {
			SmartPtr<FuncBase> f = func;
			return Functor<R(Args...)>([=](Args... args) {
				static Map<std::tuple<Args...>, R> map;
				typename Map<std::tuple<Args...>, R>::iterator it = map.find(std::make_tuple(args...));
				if(it == map.end()) {
					return map[std::make_tuple(args...)] = f->apply(args...);
				}
				return (*it)._2;
			});
		}

		explicit operator Functor<void(Args...)>() {
			return *reinterpret_cast<Functor<void(Args...)> *>(this);
		}

		static constexpr uint ArgCount = sizeof...(Args);

		template<uint N>
		using ArgType = typename std::tuple_element<N, std::tuple<Args...>>::type;

	private:
		mutable SmartPtr<FuncBase> func;

};

}
}

#endif // FUNCTOR_H
