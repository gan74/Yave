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
#include "helper.h"

namespace y {

namespace io2 {
class Reader;
class Writer;
}

namespace serde2 {

// TODO: We still pay a virtual call by read/write op, change that if possible

class ReadableArchive final {

	public:
		ReadableArchive(io2::Reader&& reader) : _reader(std::move(reader)) {
		}

		template<typename T>
		explicit ReadableArchive(T&& t) : _reader(y_fwd(t)) {
		}


		template<typename T, typename... Args>
		Result operator()(T&& t, Args&&... args) {
			return process(y_fwd(t), y_fwd(args)...);
		}

		template<typename T>
		Result array(T* t, usize n) {
			return helper::deserialize_array(*this, t, n);
		}

		io2::Reader& reader() {
			return _reader;
		}

	private:
		template<typename T, typename... Args>
		Result process(T&& t, Args&&... args) {
			return chain(helper::deserialize_one(*this, y_fwd(t)), y_fwd(args)...);
		}

		template<typename... Args>
		Result chain(Result res, Args&&... args) {
			if(!res) {
				return res;
			}
			if constexpr(sizeof...(Args)) {
				return process(y_fwd(args)...);
			}
			return core::Ok();
		}

		io2::Reader _reader;
};


class WritableArchive final {

	public:
		WritableArchive(io2::Writer&& writer) : _writer(std::move(writer)) {
		}

		template<typename T>
		explicit WritableArchive(T&& t) : _writer(y_fwd(t)) {
		}


		template<typename T, typename... Args>
		Result operator()(const T& t, const Args&... args) {
			return process(t, args...);
		}

		template<typename T>
		Result array(const T* t, usize n) {
			return helper::serialize_array(*this, t, n);
		}

		io2::Writer& writer() {
			return _writer;
		}

	private:
		template<typename T, typename... Args>
		Result process(const T& t, const Args&... args) {
			return chain(helper::serialize_one(*this, t), args...);
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

		io2::Writer _writer;
};


// -------------------------------------- tests --------------------------------------
namespace {
struct Serial {
	u32 i;
	y_serde2(i)
};
static_assert(helper::has_serialize_v<WritableArchive, Serial>);
static_assert(helper::has_serialize_v<WritableArchive, Serial&>);
static_assert(helper::has_serialize_v<WritableArchive, const Serial&>);

static_assert(helper::has_deserialize_v<ReadableArchive, Serial>);
static_assert(helper::has_deserialize_v<ReadableArchive, Serial&>);
static_assert(helper::has_deserialize_v<ReadableArchive, Serial&&>);
}


}
}

#endif // Y_SERDE2_ARCHIVES_H
