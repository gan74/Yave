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
#ifndef Y_SERDE2_ARCHIVES_H
#define Y_SERDE2_ARCHIVES_H

#include "serde.h"
#include "formats.h"

#include <y/io2/io.h>

namespace y {
namespace serde2 {


namespace helper {

// -------------------------------------- deserialize --------------------------------------

template<typename Arc, typename T>
static Result deserialize_array(Arc& ar, T* t, usize n) {
	if constexpr(is_deserializable<Arc, T>::value) {
		for(usize i = 0; i != n; ++i) {
			if(!t[i].deserialize(ar)) {
				return core::Err();
			}
		}
		return core::Ok();
	} else {
		return ar.reader().read_array(t, n);
	}
}

template<typename Arc, typename T>
static Result deserialize(Arc& ar, T& t) {
	if constexpr(is_deserializable<Arc, T>::value) {
		return t.deserialize(ar);
	} else {
		return ar.reader().read_one(t);
	}
}

template<typename Arc, typename T, typename... Args>
static Result deserialize(Arc& ar, core::Vector<T, Args...>& t) {
	u64 size = 0;
	if(!deserialize(ar, size)) {
		return core::Err();
	}
	t.make_empty();
	t.set_min_capacity(size);
	for(u64 i = 0; i != size; ++i) {
		t.emplace_back();
	}
	return deserialize_array(ar, t.data(), size);
}

template<typename Arc>
static Result deserialize(Arc& ar, core::String& t) {
	u64 size = 0;
	if(!deserialize(ar, size)) {
		return core::Err();
	}
	t = core::String(nullptr, size);
	return deserialize_array(ar, t.data(), size);
}

template<usize I, typename Arc, typename... Args>
static Result deserialize_tuple_elem(Arc& ar, std::tuple<Args...>& tpl) {
	if constexpr(I < sizeof...(Args)) {
		if(!deserialize(ar, std::get<I>(tpl))) {
			return core::Err();
		}
		return deserialize_tuple_elem<I + 1>(ar, tpl);
	}
	return core::Ok();
}

template<typename Arc, typename... Args>
static Result deserialize(Arc& ar, std::tuple<Args...>& tpl) {
	return deserialize_tuple_elem<0>(ar, tpl);
}


// -------------------------------------- serialize --------------------------------------

template<typename Arc, typename T>
static Result serialize_array(Arc& ar, const T* t, usize n) {
	if constexpr(is_serializable<Arc, T>::value) {
		for(usize i = 0; i != n; ++i) {
			if(!t[i].serialize(ar)) {
				return core::Err();
			}
		}
		return core::Ok();
	} else {
		return ar.writer().write_array(t, n);
	}
}

template<typename Arc, typename T>
static Result serialize(Arc& ar, const T& t) {
	if constexpr(is_serializable<Arc, T>::value) {
		return t.serialize(ar);
	} else {
		return ar.writer().write_one(t);
	}
}

template<typename Arc, typename T>
static Result serialize(Arc& ar, core::ArrayView<T> t) {
	if(!serialize(ar, u64(t.size()))) {
		return core::Err();
	}
	return serialize_array(ar, t.data(), t.size());
}

template<typename Arc, typename T, typename... Args>
static Result serialize(Arc& ar, const core::Vector<T, Args...>& t) {
	return serialize(ar, core::ArrayView(t));
}

template<typename Arc>
static Result serialize(Arc& ar, const core::String& t) {
	return serialize(ar, core::ArrayView<core::String::value_type>(t));
}

template<usize I, typename Arc, typename... Args>
static Result serialize_tuple_elem(Arc& ar, const std::tuple<Args...>& tpl) {
	if constexpr(I < sizeof...(Args)) {
		if(!serialize(ar, std::get<I>(tpl))) {
			return core::Err();
		}
		return serialize_tuple_elem<I + 1>(ar, tpl);
	}
	return core::Ok();
}

template<typename Arc, typename... Args>
static Result serialize(Arc& ar, const std::tuple<Args...>& tpl) {
	return serialize_tuple_elem<0>(ar, tpl);
}


}


// -------------------------------------- archives --------------------------------------

template<typename Format = BinaryFormat>
class ReadableArchive final {
	public:
		ReadableArchive(io2::Reader& reader) : _reader(reader) {
		}

		template<typename T, typename... Args>
		Result operator()(T& t, Args&... args) {
			return process(t, args...);
		}

		template<typename T>
		Result array(T* t, usize n) {
			return helper::deserialize_array(*this, t, n);
		}

		FormattedReader<Format>& reader() {
			return _reader;
		}

	private:
		template<typename T, typename... Args>
		Result process(T& t, Args&... args) {
			return chain(helper::deserialize(*this, t), args...);
		}

		template<typename... Args>
		Result chain(Result res, Args&... args) {
			if(!res) {
				return res;
			}
			if constexpr(sizeof...(Args)) {
				return process(args...);
			}
			return core::Ok();
		}

		FormattedReader<Format> _reader;
};


template<typename Format = BinaryFormat>
class WritableArchive final {
	public:
		WritableArchive(io2::Writer& writer) : _writer(writer) {
		}

		template<typename T, typename... Args>
		Result operator()(const T& t, const Args&... args) {
			return process(t, args...);
		}

		template<typename T>
		Result array(const T* t, usize n) {
			return helper::serialize_array(*this, t, n);
		}

		FormattedWriter<Format>& writer() {
			return _writer;
		}

	private:
		template<typename T, typename... Args>
		Result process(const T& t, const Args&... args) {
			return chain(helper::serialize(*this, t), args...);
		}

		template<typename... Args>
		Result chain(Result res, const Args&... args) {
			if(!res) {
				return res;
			}
			if constexpr(sizeof...(Args)) {
				return process(args...);
			}
			return core::Ok();
		}

		FormattedWriter<Format> _writer;
};


}
}

#endif // Y_SERDE2_ARCHIVES_H
