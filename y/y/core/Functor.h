/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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

	virtual Ret apply(Args&&...) = 0;

};

template<typename T, typename Ret, typename... Args>
struct Function : FunctionBase<Ret, Args...> {
	public:
		Function(T&& t) : _func(std::forward<T>(t)) {
		}

		Ret apply(Args&&... args) override {
			if constexpr(std::is_void_v<Ret>) {
				_func(std::forward<Args>(args)...);
			} else {
				return _func(std::forward<Args>(args)...);
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
		Functor(T func) : _function(std::make_unique<Function<T, Ret, Args...>>(std::move(func))) {
		}

		Functor(Functor&& other) : _function(nullptr) {
			std::swap(_function, other._function);
		}

		Functor(const Functor& other) : _function(other._function) {
		}

		Functor& operator=(Functor&& other) {
			std::swap(_function, other._function);
			return *this;
		}

		Functor& operator=(const Functor& other) {
			_function = other._function;
			return *this;
		}

		bool operator==(const Functor& other) const {
			return _function == other._function;
		}

		bool operator!=(const Functor& other) const {
			return _function != other._function;
		}

		Ret operator()(Args&&... args) const {
			return _function->apply(std::forward<Args>(args)...);
		}

	private:
		Container<FunctionBase<Ret, Args...>> _function;
};

// http://stackoverflow.com/questions/7943525/is-it-possible-to-figure-out-the-parameter-type-and-return-type-of-a-lambda
template<typename T>
struct functor_type : functor_type<decltype(&T::operator())> {
};

template<typename Ret, typename... Args>
struct functor_type<Ret(*)(Args...)> {

	template<template<typename...> typename Container>
	using type = Container<Ret(Args...)>;
	using traits = function_traits<Ret(Args...)>;
};

template<typename Ret, typename... Args>
struct functor_type<Ret(&)(Args...)> {

	template<template<typename...> typename Container>
	using type = Container<Ret(Args...)>;
	using traits = function_traits<Ret(Args...)>;
};

template<typename Ret, typename... Args>
struct functor_type<Ret(Args...)> {

	template<template<typename...> typename Container>
	using type = Container<Ret(Args...)>;
	using traits = function_traits<Ret(Args...)>;
};

template<typename T, typename Ret, typename... Args>
struct functor_type<Ret(T::*)(Args...) const> {

	template<template<typename...> typename Container>
	using type = Container<Ret(Args...)>;
	using traits = function_traits<Ret(Args...)>;
};

template<typename T, typename Ret, typename... Args>
struct functor_type<Ret(T::*)(Args...)> {

	template<template<typename...> typename Container>
	using type = Container<Ret(Args...)>;
	using traits = function_traits<Ret(Args...)>;
};

}




template<typename Ret, typename... Args>
using Function = detail::Functor<std::unique_ptr, Ret, Args...>;

template<typename Ret, typename... Args>
using Functor = detail::Functor<std::shared_ptr, Ret, Args...>;




template<typename T>
inline auto function(T&& func) {
	return typename detail::functor_type<typename std::remove_reference<T>::type>::template type<Function>(std::forward<T>(func));
}

template<typename T>
inline auto functor(T&& func) {
	return typename detail::functor_type<typename std::remove_reference<T>::type>::template type<Functor>(std::forward<T>(func));
}

}
}

#endif // Y_CORE_FUNCTOR_H
