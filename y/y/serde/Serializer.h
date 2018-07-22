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
#ifndef Y_SERDE_SERIALIZER_H
#define Y_SERDE_SERIALIZER_H

#include <y/io/Writer.h>
#include <y/io/Ref.h>

#include <y/core/String.h>

namespace y {
namespace serde {

namespace detail {
template<typename T>
static auto has_serialize(T*) -> bool_type<std::is_void_v<decltype(std::declval<T>().serialize(std::declval<io::WriterRef>()))>>;
template<typename T>
static auto has_serialize(...) -> std::false_type;
}

template<typename T>
using is_serializable = bool_type<
		decltype(detail::has_serialize<T>(nullptr))::value &&
		decltype(detail::has_serialize<T>(nullptr))::value
	>;



template<typename T>
void serialize(io::WriterRef writer, const T& t);
template<typename T>
void serialize_array(io::WriterRef writer, T* arr, usize size);



template<typename T>
struct Serializer {
	static void serialize(io::WriterRef writer, const T& t) {
		static_assert(!std::is_pointer_v<T> && !std::is_array_v<T>);

		if constexpr(is_serializable<T>::value) {
			t.serialize(writer);
		} else {
			writer->write_one(t);
		}
	}
};

template<typename... Args>
struct Serializer<std::tuple<Args...>> {
	template<usize I>
	static void serialize_one(io::WriterRef writer, const std::tuple<Args...>& t) {
		if constexpr(I < sizeof...(Args)) {
			using element = std::tuple_element_t<I, std::tuple<Args...>>;
			Serializer<element>::serialize(writer, std::get<I>(t));
			serialize_one<I + 1>(writer, t);
		}
	}

	static void serialize(io::WriterRef writer, const std::tuple<Args...>& t) {
		serialize_one<0>(writer, t);
	}
};

template<typename T>
struct Serializer<core::ArrayView<T>> {
	static void serialize(io::WriterRef writer, core::ArrayView<T> arr) {
		Serializer<u64>::serialize(writer, u64(arr.size()));
		serialize_array(writer, arr.begin(), arr.size());
	}
};

template<>
struct Serializer<std::string_view> {
	static void serialize(io::WriterRef writer, std::string_view str) {
		using arr_t = core::ArrayView<std::string_view::value_type>;
		Serializer<arr_t>::serialize(writer, arr_t(str.data(), str.size()));
	}
};

template<>
struct Serializer<core::String> {
	static void serialize(io::WriterRef writer, const core::String& str) {
		Serializer<std::string_view>::serialize(writer, str);
	}
};

template<typename T>
struct Serializer<core::Vector<T>> {
	static void serialize(io::WriterRef writer, const core::Vector<T>& vec) {
		Serializer<core::ArrayView<T>>::serialize(writer, vec);
	}
};




template<typename T>
void serialize(io::WriterRef writer, const T& t) {
	Serializer<T>::serialize(writer, t);
}

template<typename T>
void serialize_array(io::WriterRef writer, T* arr, usize size) {
	if constexpr(is_serializable<T>::value) {
		for(usize i = 0; i != size; ++i) {
			arr[i].serialize(writer);
		}
	} else {
		writer->write_array(arr, size);
	}
}

}
}

#endif // Y_SERDE_SERIALIZER_H
