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

#include <y/utils.h>

namespace y {

namespace core {
class String;
template<typename Elem, typename ResizePolicy, typename Allocator>
class Vector;
}


namespace serde2 {

using Result = core::Result<void>;



// -------------------------------------- deserialize --------------------------------------
namespace helper {

template<typename T>
struct Deserializer {};

template<typename T>
struct Deserializer<T&> : Deserializer<T> {};

template<typename T>
struct Deserializer<T&&> : Deserializer<T> {};


template<typename Arc, typename T>
static Result deserialize_one(Arc& ar, T&& t);

template<typename Arc, typename T>
static Result deserialize_array(Arc& ar, T* t, usize n);

Y_TODO(deserialized ?)

namespace detail {
template<typename Arc, typename T>
static auto has_deserializer(T*) -> bool_type<std::is_same_v<Result, decltype(Deserializer<T>::deserialize(std::declval<Arc&>(), std::declval<T&>()))>>;
template<typename Arc, typename T>
static auto has_deserializer(...) -> std::false_type;

template<typename Arc, typename T>
static auto has_deserialize(T*) -> bool_type<std::is_same_v<Result, decltype(std::declval<T&>().deserialize(std::declval<Arc&>()))>>;
template<typename Arc, typename T>
static auto has_deserialize(...) -> std::false_type;
}
}


template<typename Arc, typename T, typename U = remove_cvref_t<T>>
using has_deserializer = bool_type<decltype(helper::detail::has_deserializer<Arc, U>(nullptr))::value>;
template<typename Arc, typename T, typename U = remove_cvref_t<T>>
using has_deserialize = bool_type<decltype(helper::detail::has_deserialize<Arc, U>(nullptr))::value>;
template<typename Arc, typename T>
using is_deserializable = bool_type<has_deserialize<Arc, T>::value || has_deserializer<Arc, T>::value || std::is_trivially_copyable_v<T>>;


template<typename Arc, typename T>
static constexpr bool has_deserializer_v = has_deserializer<Arc, T>::value;
template<typename Arc, typename T>
static constexpr bool has_deserialize_v = has_deserialize<Arc, T>::value;
template<typename Arc, typename T>
static constexpr bool is_deserializable_v = is_deserializable<Arc, T>::value;


namespace helper {
template<typename Arc, typename T>
static Result deserialize_one(Arc& ar, T&& t) {
	static_assert(!(has_deserializer_v<Arc, T> && has_deserialize_v<Arc, T>));

	if constexpr(has_deserialize_v<Arc, T>) {
		return t.deserialize(ar);
	} else if constexpr(has_deserializer_v<Arc, T>){
		return Deserializer<T>::deserialize(ar, t);
	} else {
		static_assert(!std::is_pointer_v<T>);
		return ar.reader().read_one(t);
	}
}

template<typename Arc, typename T>
static Result deserialize_array(Arc& ar, T* t, usize n) {
	if constexpr(has_deserializer_v<Arc, T> || has_deserialize_v<Arc, T>) {
		for(usize i = 0; i != n; ++i) {
			if(!deserialize_one(ar, t[i])) {
				return core::Err();
			}
		}
		return core::Ok();
	} else {
		return ar.reader().read_array(t, n);
	}
}

}





// -------------------------------------- serialize --------------------------------------
namespace helper {

template<typename T>
struct Serializer {};

template<typename T>
struct Serializer<const T&> : Serializer<T> {};

template<typename T>
struct Serializer<T&&> : Serializer<T> {};


template<typename Arc, typename T>
static Result serialize_one(Arc& ar, const T& t);

template<typename Arc, typename T>
static Result serialize_array(Arc& ar, const T* t, usize n);


namespace detail {
template<typename Arc, typename T, typename U = remove_cvref_t<T>>
static auto has_serializer(T*) -> bool_type<std::is_same_v<Result, decltype(Serializer<U>::serialize(std::declval<Arc&>(), std::declval<const U&>()))>>;
template<typename Arc, typename T>
static auto has_serializer(...) -> std::false_type;

template<typename Arc, typename T>
static auto has_serialize(T*) -> bool_type<std::is_same_v<Result, decltype(std::declval<const T&>().serialize(std::declval<Arc&>()))>>;
template<typename Arc, typename T>
static auto has_serialize(...) -> std::false_type;
}
}


template<typename Arc, typename T, typename U = remove_cvref_t<T>>
using has_serializer = bool_type<decltype(helper::detail::has_serializer<Arc, U>(nullptr))::value>;
template<typename Arc, typename T, typename U = remove_cvref_t<T>>
using has_serialize = bool_type<decltype(helper::detail::has_serialize<Arc, U>(nullptr))::value>;
template<typename Arc, typename T>
using is_serializable = bool_type<has_serialize<Arc, T>::value || has_serializer<Arc, T>::value || std::is_trivially_copyable_v<T>>;


template<typename Arc, typename T>
static constexpr bool has_serializer_v = has_serializer<Arc, T>::value;
template<typename Arc, typename T>
static constexpr bool has_serialize_v = has_serialize<Arc, T>::value;
template<typename Arc, typename T>
static constexpr bool is_serializable_v = is_serializable<Arc, T>::value;


namespace helper {
template<typename Arc, typename T>
static Result serialize_one(Arc& ar, const T& t) {
	static_assert(!(has_serializer_v<Arc, T> && has_serialize_v<Arc, T>));

	if constexpr(has_serialize_v<Arc, T>) {
		return t.serialize(ar);
	} else if constexpr(has_serializer_v<Arc, T>){
		return Serializer<T>::serialize(ar, t);
	} else {
		static_assert(!std::is_pointer_v<T>);
		return ar.writer().write_one(t);
	}
}

template<typename Arc, typename T>
static Result serialize_array(Arc& ar, const T* t, usize n) {
	if constexpr(has_serializer_v<Arc, T> || has_serialize_v<Arc, T>) {
		for(usize i = 0; i != n; ++i) {
			if(!serialize_one(ar, t[i])) {
				return core::Err();
			}
		}
		return core::Ok();
	} else {
		return ar.writer().write_array(t, n);
	}
}
}






// -------------------------------------- deserializers --------------------------------------
namespace helper {
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
}


// -------------------------------------- serializers --------------------------------------
namespace helper {
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
}


}
}

#endif // Y_SERDE2_HELPER_H
