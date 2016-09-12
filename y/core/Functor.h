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
#ifndef Y_CORE_FUNCTION_H
#define Y_CORE_FUNCTION_H

#include <y/utils.h>
#include "Ptr.h"

namespace y {
namespace core {

namespace detail {

template<typename Ret, typename... Args>
struct FunctionBase : NonCopyable {
	FunctionBase() {
	}

	virtual ~FunctionBase() {
	}

	virtual Ret apply(Args...) = 0;
	virtual Ret apply(Args...) const = 0;
};

template<typename T, typename Ret, typename... Args>
struct Function : FunctionBase<Ret, Args...> {
	public:
		Function(const T& t) : f(t) {
		}

		virtual Ret apply(Args... args) override {
			return f(args...);
		}

		virtual Ret apply(Args... args) const override {
			return f(args...);
		}


	private:
		T f;
};

// for boxing non void functions in void functors
template<typename T, typename... Args>
struct Function<T, void, Args...> : FunctionBase<void, Args...> {
	public:
		Function(const T& t) : f(t) {
		}

		virtual void apply(Args... args) override {
			f(args...);
		}

		virtual void apply(Args... args) const override {
			f(args...);
		}


	private:
		T f;
};

template<template<typename...> typename Container, typename Ret, typename... Args>
class Functor {};

template<template<typename...> typename Container, typename Ret, typename... Args>
class Functor<Container, Ret(Args...)> {
	public:
		template<typename T>
		Functor(const T& func) : function(new Function<T, Ret, Args...>(func)) {
		}


		Functor& operator=(const Functor &) = default;
		Functor(const Functor &) = default;


		Functor(Functor&& other) : function(nullptr) {
			std::swap(function, other.function);
		}

		Functor& operator=(Functor&& other) {
			std::swap(function, other.function);
			return *this;
		}


		Ret operator()(Args... args) {
			return function->apply(args...);
		}

		Ret operator()(Args... args) const {
			return function->apply(args...);
		}

	private:
		Container<FunctionBase<Ret, Args...>> function;
};


// http://stackoverflow.com/questions/7943525/is-it-possible-to-figure-out-the-parameter-type-and-return-type-of-a-lambda
template <typename T>
struct functor_t : functor_t<decltype(&T::operator())> {
};

template<typename T, typename Ret, typename... Args>
struct functor_t<Ret(T::*)(Args...) const> {

	template<template<typename...> typename Container>
	using type = Container<Ret(Args...)>;
};

}




template<typename Ret, typename... Args>
using Function = detail::Functor<Ptr, Ret, Args...>;

template<typename Ret, typename... Args>
using Functor = detail::Functor<Rc, Ret, Args...>;




template<typename T>
inline auto function(const T& func) {
	return typename detail::functor_t<T>::template type<Function>(func);
}

template<typename T>
inline auto functor(const T& func) {
	return typename detail::functor_t<T>::template type<Functor>(func);
}



}
}

#endif // Y_CORE_FUNCTION_H
