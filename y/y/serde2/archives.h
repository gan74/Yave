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

#include "helper.h"
#include <y/core/String.h>

namespace y {

Y_TODO(We still pay a virtual call by read/write op, change that if possible)
namespace io2 {
class Reader;
class Writer;
}

namespace serde2 {

struct ReadableArchiveTag {};
struct WritableArchiveTag {};


template<typename T>
using is_readable_archive = std::is_base_of<ReadableArchiveTag, std::decay_t<T>>;
template<typename T>
static constexpr bool is_readable_archive_v = is_readable_archive<T>::value;

template<typename T>
using is_writable_archive = std::is_base_of<WritableArchiveTag, std::decay_t<T>>;
template<typename T>
static constexpr bool is_writable_archive_v = is_writable_archive<T>::value;


template<typename Derived>
class ReadableArchiveBase : public ReadableArchiveTag {

	public:
		ReadableArchiveBase(io2::Reader& reader) : _reader(reader) {
		}

		template<typename T, typename... Args>
		Result operator()(T&& t, Args&&... args) {
			return process(y_fwd(t), y_fwd(args)...);
		}

		template<typename T>
		Result array(T* t, usize n) {
			return helper::deserialize_array(static_cast<Derived&>(*this), t, n);
		}

		io2::Reader& reader() {
			return _reader;
		}

	private:
		template<typename T, typename... Args>
		Result process(T&& t, Args&&... args) {
			return chain(helper::deserialize_one(static_cast<Derived&>(*this), y_fwd(t)), y_fwd(args)...);
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

		io2::Reader& _reader;
};

template<typename Derived>
class WritableArchiveBase : public WritableArchiveTag {

	public:
		WritableArchiveBase(io2::Writer& writer) : _writer(writer) {
		}


		template<typename T, typename... Args>
		Result operator()(const T& t, const Args&... args) {
			return process(t, args...);
		}

		template<typename T>
		Result array(const T* t, usize n) {
			return helper::serialize_array(static_cast<Derived&>(*this), t, n);
		}

		io2::Writer& writer() {
			return _writer;
		}

	private:
		template<typename T, typename... Args>
		Result process(const T& t, const Args&... args) {
			return chain(helper::serialize_one(static_cast<Derived&>(*this), t), args...);
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

		io2::Writer& _writer;
};


struct ReadableArchive : ReadableArchiveBase<ReadableArchive> {
	using ReadableArchiveBase<ReadableArchive>::ReadableArchiveBase;
};

struct WritableArchive : WritableArchiveBase<WritableArchive> {
	using WritableArchiveBase<WritableArchive>::WritableArchiveBase;
};

}
}

#endif // Y_SERDE2_ARCHIVES_H
