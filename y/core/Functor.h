/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef Y_CORE_FUNCTION_H
#define Y_CORE_FUNCTION_H

#include <y/utils.h>
#include "Ptr.h"

namespace y {
namespace core {

namespace detail {

template<typename T>
struct FuncPtr {
	using type = T;
};

template<typename R, typename... Args>
struct FuncPtr<R(Args...)> {
	using type = R(*)(Args...);
};

template<typename Ret, typename... Args>
struct FunctionBase : NonCopyable {
	FunctionBase() {
	}

	virtual ~FunctionBase() {
	}

	virtual Ret apply(Args&&...) = 0;
	//virtual Ret apply(Args&&...) const = 0;
};

template<typename T, typename Ret, typename... Args>
struct Function : FunctionBase<Ret, Args...> {
	public:
		Function(T&& t) : _func(std::forward<T>(t)) {
		}

		virtual Ret apply(Args&&... args) override {
			return _func(std::forward<Args>(args)...);
		}

		/*virtual Ret apply(Args&&... args) const override {
			return _func(std::forward<Args>(args)...);
		}*/


	private:
		typename FuncPtr<T>::type _func;
};

// for boxing non void functions in void functors
template<typename T, typename... Args>
struct Function<T, void, Args...> : FunctionBase<void, Args...> {
	public:
		Function(T&& t) : _func(std::forward<T>(t)) {
		}

		virtual void apply(Args&&... args) override {
			_func(std::forward<Args>(args)...);
		}

		/*virtual void apply(Args&&... args) const override {
			_func(std::forward<Args>(args)...);
		}*/


	private:
		typename FuncPtr<T>::type _func;
};

template<template<typename...> typename Container, typename Ret, typename... Args>
class Functor {};

template<template<typename...> typename Container, typename Ret, typename... Args>
class Functor<Container, Ret(Args...)> {
	public:
		template<typename T>
		Functor(T&& func) : _function(new Function<T, Ret, Args...>(std::forward<T>(func))) {
		}


		Functor& operator=(const Functor&) = default;
		Functor(const Functor&) = default;


		Functor(Functor&& other) : _function(nullptr) {
			std::swap(_function, other._function);
		}

		Functor& operator=(Functor&& other) {
			std::swap(_function, other._function);
			return *this;
		}


		Ret operator()(Args&&... args) {
			return _function->apply(std::forward<Args>(args)...);
		}

		/*Ret operator()(Args&&... args) const {
			return _function->apply(std::forward<Args>(args)...);
		}*/

	private:
		Container<FunctionBase<Ret, Args...>> _function;
};


// http://stackoverflow.com/questions/7943525/is-it-possible-to-figure-out-the-parameter-type-and-return-type-of-a-lambda
template<typename T>
struct functor_t : functor_t<decltype(&T::operator())> {
};

template<typename Ret, typename... Args>
struct functor_t<Ret(*)(Args...)> {

	template<template<typename...> typename Container>
	using type = Container<Ret(Args...)>;
};

template<typename Ret, typename... Args>
struct functor_t<Ret(&)(Args...)> {

	template<template<typename...> typename Container>
	using type = Container<Ret(Args...)>;
};

template<typename Ret, typename... Args>
struct functor_t<Ret(Args...)> {

	template<template<typename...> typename Container>
	using type = Container<Ret(Args...)>;
};

template<typename T, typename Ret, typename... Args>
struct functor_t<Ret(T::*)(Args...) const> {

	template<template<typename...> typename Container>
	using type = Container<Ret(Args...)>;
};

template<typename T, typename Ret, typename... Args>
struct functor_t<Ret(T::*)(Args...)> {

	template<template<typename...> typename Container>
	using type = Container<Ret(Args...)>;
};

}




template<typename Ret, typename... Args>
using Function = detail::Functor<Unique, Ret, Args...>;

template<typename Ret, typename... Args>
using Functor = detail::Functor<Rc, Ret, Args...>;




template<typename T>
inline auto function(T&& func) {
	return typename detail::functor_t<typename std::remove_reference<T>::type>::template type<Function>(std::forward<T>(func));
}

template<typename T>
inline auto functor(T&& func) {
	return typename detail::functor_t<typename std::remove_reference<T>::type>::template type<Functor>(std::forward<T>(func));
}



}
}

#endif // Y_CORE_FUNCTION_H
