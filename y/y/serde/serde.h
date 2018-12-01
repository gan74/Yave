/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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
#ifndef Y_SERDE_SERDE_H
#define Y_SERDE_SERDE_H

#include <y/utils.h>
#include "Serializer.h"
#include "Deserializer.h"

#include <type_traits>


#define y_serialize(...)														\
	void serialize(y::io::WriterRef _y_serde_driver) const {					\
		auto y_serde_process = [&_y_serde_driver](auto&&... args) {				\
			y::serde::serialize_all(_y_serde_driver, 							\
				std::forward<decltype(args)>(args)...);							\
		};																		\
		y_serde_process(__VA_ARGS__);											\
	}

#define y_deserialize(...)														\
	static auto _y_serde_self_type_helper() ->									\
		std::remove_reference<decltype(*this)>::type;							\
	void deserialize(y::io::ReaderRef _y_serde_driver) {						\
		auto y_serde_process = [&_y_serde_driver](auto&&... args) {				\
			y::serde::deserialize_all(_y_serde_driver,							\
				std::forward<decltype(args)>(args)...);							\
		};																		\
		y_serde_process(__VA_ARGS__);											\
	}																			\
	template<typename = void>													\
	static auto deserialized(y::io::ReaderRef _y_serde_driver) {				\
		using self = decltype(_y_serde_self_type_helper());						\
		static_assert(std::is_default_constructible_v<self>,					\
			"auto generatred 'deserialized' is only "							\
			"available for default constructible types");						\
		self obj;																\
		obj.deserialize(_y_serde_driver);										\
		return obj;																\
	}

#define y_deserialize_func(func)												\
	static auto deserialized(y::io::ReaderRef _y_serde_driver) {				\
		return y::serde::deserialize_call(_y_serde_driver, func);				\
	}																			\
	template<typename = void>													\
	void deserialize(y::io::ReaderRef _y_serde_driver) {						\
		*this = deserialized(_y_serde_driver);									\
	}



#define y_serde(...)															\
	y_serialize(__VA_ARGS__)													\
	y_deserialize(__VA_ARGS__)													\


#define y_serde_fixed_array(size, array)										\
	y_serde_call([&] {															\
		y::serde::detail::process_fixed_array(									\
			 _y_serde_driver, (array), (size)									\
		);																		\
	})

#define y_serde_call(func) y::serde::detail::Callable(func)
#define y_serde_cond(cond, ...) y::serde::detail::Callable([&y_serde_process, this]() { if(cond) { y_serde_process(__VA_ARGS__); }; })

#define y_serde_assert(cond) do { if(!(cond)) { y_throw("Serde error: assertion failed " #cond); } } while(false)

namespace y {
namespace serde {

namespace detail {
template<typename T>
struct Callable : NonCopyable {
	Callable(T&& t) : call(y_fwd(t)) {
	}
	T call;
};

template<typename T>
void process_fixed_array(io::ReaderRef reader, T* arr, usize size) {
	deserialize_array(reader, arr, size);
}

template<typename T>
void process_fixed_array(io::WriterRef writer, const T* arr, usize size) {
	serialize_array(writer, arr, size);
}


template<typename T>
std::true_type is_callable(const Callable<T>&);
template<typename T>
std::false_type is_callable(const T&);
template<typename T>
static constexpr bool is_callable_v = decltype(is_callable(std::declval<T>()))::value;


template<typename T>
std::false_type is_const_ref(T&);
template<typename T>
std::true_type is_const_ref(const T&);
template<typename T>
static constexpr bool is_const_ref_v = decltype(is_const_ref(std::declval<T>()))::value;
}



template<typename T, typename... Args>
void serialize_all(io::WriterRef writer, T&& t, Args&&... args) {
	if constexpr(detail::is_callable_v<T>) {
		if constexpr(std::is_void_v<decltype(t.call())>) {
			t.call();
		} else {
			serialize(writer, t.call());
		}
	} else {
		serialize(writer, y_fwd(t));
	}

	if constexpr(sizeof...(args)) {
		serialize_all(writer, y_fwd(args)...);
	}
}


template<typename Func>
auto deserialize_call(io::ReaderRef reader, Func&& func) {
	typename function_traits<Func>::argument_pack a;
	deserialize(reader, a);
	return std::apply(func, std::move(a));
}

template<typename T, typename... Args>
void deserialize_all(io::ReaderRef reader, T&& t, Args&&... args) {
	if constexpr(detail::is_callable_v<T>) {
		deserialize_call(reader, std::move(t.call));
	} else {
		if constexpr(detail::is_const_ref_v<decltype(t)>) {
			using type = std::remove_const_t<std::remove_reference_t<T>>;
			auto check = deserialized<type>(reader);
			if(check != t) {
				y_throw("Deserialisation error: invalid constant.");
			}
		} else {
			deserialize(reader, y_fwd(t));
		}
	}
	if constexpr(sizeof...(args)) {
		deserialize_all(reader, y_fwd(args)...);
	}
}


}
}

#endif // Y_SERDE_SERDE_H
