/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#ifndef Y_CORE_FUNCTOR_H
#define Y_CORE_FUNCTOR_H

#include <y/utils.h>
#include <y/utils/traits.h>

#include <memory>

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
	virtual Ret apply_const(Args...) const = 0;

};

template<typename T, typename Ret, typename... Args>
struct Function : FunctionBase<Ret, Args...> {
	public:
		Function(T&& t) : _func(y_fwd(t)) {
		}

		Ret apply(Args... args) override {
			if constexpr(std::is_void_v<Ret>) {
				_func(y_fwd(args)...);
			} else {
				return _func(y_fwd(args)...);
			}
		}

		Ret apply_const(Args... args) const override {
			if constexpr(std::is_void_v<Ret>) {
				_func(y_fwd(args)...);
			} else {
				return _func(y_fwd(args)...);
			}
		}

	private:
		T _func;
};


template<template<typename...> typename Container, typename Ret, typename... Args>
class Functor {};

template<typename T>
struct is_functor {
	static constexpr bool value = false;
};

template<template<typename...> typename Container, typename Ret, typename... Args>
struct is_functor<Functor<Container, Ret(Args...)>> {
	static constexpr bool value = true;
};


template<template<typename...> typename Container, typename Ret, typename... Args>
class Functor<Container, Ret(Args...)> {
	public:
		template<typename T, typename = std::enable_if_t<!is_functor<std::remove_reference_t<T>>::value>>
		Functor(T&& func) : _function(std::make_unique<Function<T, Ret, Args...>>(y_fwd(func))) {
		}

		Functor(Functor&&) = default;
		Functor& operator=(Functor&&) = default;
		Functor& operator=(const Functor&) = default;

		Functor(const Functor& other) : _function(other._function) {
		}

		bool operator==(const Functor& other) const {
			return _function == other._function;
		}

		bool operator!=(const Functor& other) const {
			return _function != other._function;
		}

		Ret operator()(Args... args) {
			return _function->apply(y_fwd(args)...);
		}

		Ret operator()(Args... args) const {
			return _function->apply_const(y_fwd(args)...);
		}

	private:
		Container<FunctionBase<Ret, Args...>> _function;
};

}




template<typename Ret, typename... Args>
using Function = detail::Functor<std::unique_ptr, Ret, Args...>;

template<typename Ret, typename... Args>
using Functor = detail::Functor<std::shared_ptr, Ret, Args...>;

}
}

#endif // Y_CORE_FUNCTOR_H
