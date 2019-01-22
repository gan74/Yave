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
#ifndef Y_SERDE2_FORMATS_H
#define Y_SERDE2_FORMATS_H

#include "serde.h"

namespace y {
namespace serde2 {

struct BinaryFormat {
	template<typename R, typename T>
	auto read_array(R& reader, T* t, usize n) {
		static_assert(std::is_trivially_copyable_v<T>);
		return reader.read(reinterpret_cast<u8*>(t), sizeof(T) * n);
	}

	template<typename R, typename T>
	auto read(R& reader, T& t) {
		static_assert(std::is_trivially_copyable_v<T>);
		return reader.read(reinterpret_cast<u8*>(&t), sizeof(T));
	}


	template<typename W, typename T>
	auto write_array(W& writer, const T* t, usize n) {
		static_assert(std::is_trivially_copyable_v<T>);
		if(auto r = write<u64>(writer, n); r.is_error()) {
			return r;
		}
		return writer.write(reinterpret_cast<const u8*>(t), sizeof(T) * n);
	}

	template<typename W, typename T>
	auto write(W& writer, const T& t) {
		static_assert(std::is_trivially_copyable_v<T>);
		return writer.write(reinterpret_cast<u8*>(&t), sizeof(T));
	}
};

}
}

#endif // Y_SERDE2_FORMATS_H
