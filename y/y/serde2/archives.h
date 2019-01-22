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

template<typename Format = BinaryFormat>
class ReadableArchive final {
	public:
		template<typename T>
		ReadableArchive(T& reader) : _reader(reader) {
		}

		template<typename T, typename... Args>
		Result operator()(T& t, Args&... args) {
			return read(t, args...);
		}

		template<typename T>
		Result array(T* t, usize n) {
			if constexpr(is_deserializable<ReadableArchive, T>::value) {
				for(usize i = 0; i != n; ++i) {
					if(auto r = t[i].deserialize(*this); r.is_error()) {
						return r;
					}
				}
				return core::Ok();
			} else {
				return _format.read_array(_reader, t, n);
			}
		}

	private:
		template<typename T, typename... Args>
		Result read(T& t, Args&... args) {
			return continue_reading(read_one(t), args...);
		}

		template<typename... Args>
		Result continue_reading(Result res, Args&... args) {
			if(res.is_error()) {
				return res;
			}
			if constexpr(sizeof...(Args)) {
				return read(args...);
			}
			return core::Ok();
		}

		template<typename T>
		Result read_one(T& t) {
			if constexpr(is_deserializable<ReadableArchive, T>::value) {
				return t.deserialize(*this);
			} else {
				if(_format.read(_reader, t).is_error()) {
					return core::Err();
				}
				return core::Ok();
			}
		}

		Format _format;
		io2::Reader& _reader;
};



template<typename Format = BinaryFormat>
class WritableArchive final {
	public:
		template<typename T>
		WritableArchive(T& writer) : _writer(writer) {
		}

		template<typename T, typename... Args>
		Result operator()(const T& t, const Args&... args) {
			return write(t, args...);
		}

		template<typename T>
		Result array(const T* t, usize n) {
			if constexpr(is_serializable<WritableArchive, T>::value) {
				for(usize i = 0; i != n; ++i) {
					if(auto r = t[i].serialize(*this); r.is_error()) {
						return r;
					}
				}
				return core::Ok();
			} else {
				return _format.write_array(_writer, t, n);
			}
		}

	private:
		template<typename T, typename... Args>
		Result write(const T& t, const Args&... args) {
			return continue_writing(write_one(t), args...);
		}

		template<typename... Args>
		Result continue_writing(Result res, const Args&... args) {
			if(res.is_error()) {
				return res;
			}
			if constexpr(sizeof...(Args)) {
				return write(args...);
			}
			return core::Ok();
		}

		template<typename T>
		Result write_one(const T& t) {
			if constexpr(is_serializable<WritableArchive, T>::value) {
				return t.serialize(*this);
			} else {
				if(_format.write(_writer, t).is_error()) {
					return core::Err();
				}
				return core::Ok();
			}
		}

		Format _format;
		io2::Writer& _writer;
};

}
}

#endif // Y_SERDE2_ARCHIVES_H
