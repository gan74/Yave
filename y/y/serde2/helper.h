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
#ifndef Y_SERDE2_HELPER_H
#define Y_SERDE2_HELPER_H

#include "formats.h"

namespace y {
namespace serde2 {


// -------------------------------------- deserialize --------------------------------------
namespace helper {

template<typename Arc, typename T>
static Result deserialize_one(Arc& ar, T& t);

template<typename Arc, typename T>
static Result deserialize_array(Arc& ar, T* t, usize n);

template<typename Arc, typename T>
static Result deserialize_one(Arc& ar, const T& t);

template<typename Arc, typename T>
static Result deserialize_array(Arc& ar, const T* t, usize n);

template<typename T>
struct Deserializer {
};

template<typename T>
struct Deserializer<std::unique_ptr<T>> {
	template<typename Arc>
	static Result deserialize(Arc& ar, std::unique_ptr<T>& t) {
		u32 not_null = 0;
		if(!deserialize_one(ar, not_null)) {
			return core::Err();
		}
		if(!not_null) {
			return core::Ok();
		}
		t = std::make_unique<T>();
		return deserialize_one(ar, *t.get());
	}
};

template<typename T, typename... Args>
struct Deserializer<core::Vector<T, Args...>> {
	template<typename Arc>
	static Result deserialize(Arc& ar, core::Vector<T, Args...>& t) {
		u64 size = 0;
		if(!deserialize_one(ar, size)) {
			return core::Err();
		}
		t.make_empty();
		t.set_min_capacity(size);
		for(u64 i = 0; i != size; ++i) {
			t.emplace_back();
		}
		return deserialize_array(ar, t.data(), size);
	}
};

template<>
struct Deserializer<core::String> {
	template<typename Arc>
	static Result deserialize(Arc& ar, core::String& t) {
		u64 size = 0;
		if(!deserialize_one(ar, size)) {
			return core::Err();
		}
		t = core::String(nullptr, size);
		return deserialize_array(ar, t.data(), size);
	}
};

template<typename... Args>
struct Deserializer<std::tuple<Args...>> {
	template<usize I, typename Arc>
	static Result deserialize_tuple_elem(Arc& ar, std::tuple<Args...>& tpl) {
		if constexpr(I < sizeof...(Args)) {
			if(!deserialize_one(ar, std::get<I>(tpl))) {
				return core::Err();
			}
			return deserialize_tuple_elem<I + 1>(ar, tpl);
		}
		return core::Ok();
	}

	template<typename Arc>
	static Result deserialize(Arc& ar, std::tuple<Args...>& tpl) {
		return deserialize_tuple_elem<0>(ar, tpl);
	}
};

namespace detail {
template<typename Arc, typename T>
static auto has_deserializer(T*) -> bool_type<std::is_same_v<Result, decltype(Deserializer<T>::deserialize(std::declval<Arc&>(), std::declval<T&>()))>>;
template<typename Arc, typename T>
static auto has_deserializer(...) -> std::false_type;

template<typename Arc, typename T>
static auto has_deserialize(T*) -> bool_type<std::is_same_v<Result, decltype(std::declval<T>().deserialize(std::declval<Arc&>()))>>;
template<typename Arc, typename T>
static auto has_deserialize(...) -> std::false_type;
}

template<typename Arc, typename T>
using has_deserializer = bool_type<decltype(detail::has_deserializer<Arc, T>(nullptr))::value>;
template<typename Arc, typename T>
using is_deserializable = bool_type<decltype(detail::has_deserialize<Arc, T>(nullptr))::value>;

template<typename Arc, typename T>
static Result deserialize_one(Arc& ar, T& t) {
	if constexpr(is_deserializable<Arc, T>::value) {
		return t.deserialize(ar);
	} else if constexpr(has_deserializer<Arc, T>::value){
		return Deserializer<T>::deserialize(ar, t);
	} else {
		return ar.reader().read_one(t);
	}
}

template<typename Arc, typename T>
static Result deserialize_array(Arc& ar, T* t, usize n) {
	if constexpr(is_deserializable<Arc, T>::value) {
		for(usize i = 0; i != n; ++i) {
			if(!deserialize_one(ar, t[i])) {
				return core::Err();
			}
		}
		return core::Ok();
	} else if constexpr(has_deserializer<Arc, T>::value) {
		for(usize i = 0; i != n; ++i) {
			if(!Deserializer<T>::deserialize(ar, t[i])) {
				return core::Err();
			}
		}
		return core::Ok();
	} else {
		return ar.reader().read_array(t, n);
	}
}

template<typename Arc, typename T>
static Result deserialize_one(Arc& ar, const T& t) {
	return t.deserialize(ar);
}

template<typename Arc, typename T>
static Result deserialize_array(Arc& ar, const T* t, usize n) {
	for(usize i = 0; i != n; ++i) {
		if(!deserialize_one(ar, t[i])) {
			return core::Err();
		}
	}
	return core::Ok();
}

}





// -------------------------------------- serialize --------------------------------------
namespace helper {

template<typename Arc, typename T>
static Result serialize_one(Arc& ar, const T& t);

template<typename Arc, typename T>
static Result serialize_array(Arc& ar, const T* t, usize n);

template<typename T>
struct Serializer {
};

template<typename T>
struct Serializer<std::unique_ptr<T>> {
	template<typename Arc>
	static Result serialize(Arc& ar, const std::unique_ptr<T>& t) {
		if(!t.get()) {
			return serialize_one(ar, u32(0));
		}
		if(!serialize_one(ar, u32(1))) {
			return core::Err();
		}
		return serialize_one(ar, *t.get());
	}
};

template<typename T>
struct Serializer<core::ArrayView<T>> {
	template<typename Arc>
	static Result serialize(Arc& ar, core::ArrayView<T> t) {
		if(!serialize_one(ar, u64(t.size()))) {
			return core::Err();
		}
		return serialize_array(ar, t.data(), t.size());
	}
};

template<typename T, typename... Args>
struct Serializer<core::Vector<T, Args...>> {
	template<typename Arc>
	static Result serialize(Arc& ar, const core::Vector<T, Args...>& t) {
		return serialize_one(ar, core::ArrayView<T>(t));
	}
};

template<>
struct Serializer<core::String> {
	template<typename Arc>
	static Result serialize(Arc& ar, const core::String& t) {
		return serialize_one(ar, core::ArrayView<core::String::value_type>(t));
	}
};

template<typename... Args>
struct Serializer<std::tuple<Args...>> {
	template<usize I, typename Arc>
	static Result serialize_tuple_elem(Arc& ar, const std::tuple<Args...>& tpl) {
		if constexpr(I < sizeof...(Args)) {
			if(!serialize_one(ar, std::get<I>(tpl))) {
				return core::Err();
			}
			return serialize_tuple_elem<I + 1>(ar, tpl);
		}
		return core::Ok();
	}

	template<typename Arc>
	static Result serialize(Arc& ar, const std::tuple<Args...>& tpl) {
		return serialize_tuple_elem<0>(ar, tpl);
	}
};

namespace detail {
template<typename Arc, typename T>
static auto has_serializer(T*) -> bool_type<std::is_same_v<Result, decltype(Serializer<T>::serialize(std::declval<Arc&>(), std::declval<const T&>()))>>;
template<typename Arc, typename T>
static auto has_serializer(...) -> std::false_type;

template<typename Arc, typename T>
static auto has_serialize(T*) -> bool_type<std::is_same_v<Result, decltype(std::declval<T>().serialize(std::declval<Arc&>()))>>;
template<typename Arc, typename T>
static auto has_serialize(...) -> std::false_type;
}

template<typename Arc, typename T>
using has_serializer = bool_type<decltype(detail::has_serializer<Arc, T>(nullptr))::value>;
template<typename Arc, typename T>
using is_serializable = bool_type<decltype(detail::has_serialize<Arc, T>(nullptr))::value>;

template<typename Arc, typename T>
static Result serialize_one(Arc& ar, const T& t) {
	if constexpr(is_serializable<Arc, T>::value) {
		return t.serialize(ar);
	} else if constexpr(has_serializer<Arc, T>::value){
		return Serializer<T>::serialize(ar, t);
	} else {
		return ar.writer().write_one(t);
	}
}

template<typename Arc, typename T>
static Result serialize_array(Arc& ar, const T* t, usize n) {
	if constexpr(is_serializable<Arc, T>::value) {
		for(usize i = 0; i != n; ++i) {
			if(!serialize_one(ar, t[i])) {
				return core::Err();
			}
		}
		return core::Ok();
	} else if constexpr(has_serializer<Arc, T>::value) {
		for(usize i = 0; i != n; ++i) {
			if(!Serializer<T>::serialize(ar, t[i])) {
				return core::Err();
			}
		}
		return core::Ok();
	} else {
		return ar.writer().write_array(t, n);
	}
}

}

}
}

#endif // Y_SERDE2_HELPER_H
