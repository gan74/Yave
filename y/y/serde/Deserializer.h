/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#ifndef Y_SERDE_DESERIALIZER_H
#define Y_SERDE_DESERIALIZER_H

#include <y/io/Reader.h>
#include <y/io/Ref.h>

#include <y/core/String.h>

namespace y {
namespace serde {

namespace detail {
template<typename T>
static auto has_deserialize(T*) -> bool_type<std::is_void_v<decltype(std::declval<T>().deserialize(std::declval<io::ReaderRef>()))>>;
template<typename T>
static auto has_deserialize(...) -> std::false_type;
}

template<typename T>
using is_deserializable = bool_type<
		decltype(detail::has_deserialize<T>(nullptr))::value &&
		decltype(detail::has_deserialize<T>(nullptr))::value
	>;


template<typename T>
void deserialize(io::ReaderRef reader, T& t);
template<typename T>
T deserialized(io::ReaderRef reader);
template<typename T>
void deserialize_array(io::ReaderRef reader, T* arr, usize size);


template<typename T>
struct Deserializer {
	static void deserialize(io::ReaderRef reader, T& t) {
		static_assert(!std::is_pointer_v<T> && !std::is_array_v<T>);
		if constexpr(is_deserializable<T>::value) {
			t.deserialize(reader);
		} else {
			reader->read_one(t);
		}
	}
};

template<typename T>
struct Deserializer<const T> {};

template<typename... Args>
struct Deserializer<std::tuple<Args...>> {
	template<usize I>
	static void deserialize_one(io::ReaderRef reader, std::tuple<Args...>& t) {
		if constexpr(I < sizeof...(Args)) {
			using element = std::tuple_element_t<I, std::tuple<Args...>>;
			Deserializer<element>::deserialize(reader, std::get<I>(t));
			deserialize_one<I + 1>(reader, t);
		}
	}

	static void deserialize(io::ReaderRef reader, std::tuple<Args...>& t) {
		deserialize_one<0>(reader, t);
	}
};

template<typename T>
struct Deserializer<core::Vector<T>> {
	static void deserialize(io::ReaderRef reader, core::Vector<T>& vec) {
		u64 size = deserialized<u64>(reader);
		if constexpr(std::is_default_constructible_v<T>) {
			vec = core::Vector(size, T());
			deserialize_array(reader, vec.data(), size);
		} else {
			vec.clear();
			vec.set_min_capacity(size);
			for(u64 i = 0; i != size; ++i) {
				if constexpr(is_deserializable<T>::value) {
					vec.emplace_back(deserialized<T>(reader));
				} else {
					vec.emplace_back();
					deserialize(reader, vec.last());
				}
			}
		}
	}
};

template<>
struct Deserializer<core::String> {
	static void deserialize(io::ReaderRef reader, core::String& str) {
		u64 size = deserialized<u64>(reader);
		str = core::String(nullptr, size);
		deserialize_array(reader, str.data(), size);
		y_debug_assert(str[size] == 0);
	}
};




template<typename T>
void deserialize(io::ReaderRef reader, T& t) {
	Deserializer<T>::deserialize(reader, t);
}

template<typename T>
T deserialized(io::ReaderRef reader) {
	if constexpr(std::is_default_constructible_v<T>) {
		T t{};
		deserialize(reader, t);
		return t;
	} else {
		return T::deserialized(reader);
	}
}

template<typename T>
void deserialize_array(io::ReaderRef reader, T* arr, usize size) {
	if constexpr(is_deserializable<T>::value) {
		for(usize i = 0; i != size; ++i) {
			arr[i].deserialize(reader);
		}
	} else {
		reader->read_array(arr, size);
	}
}


}
}


#endif // Y_SERDE_DESERIALIZER_H
