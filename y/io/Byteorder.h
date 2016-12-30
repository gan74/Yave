/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#ifndef Y_IO_BYTEORDER_H
#define Y_IO_BYTEORDER_H

#include "io.h"

namespace y {
namespace io {

enum class Byteorder {
	LittleEndian,
	BigEndian
};

constexpr Byteorder system_byteorder() {
	return is_little_endian() ? Byteorder::LittleEndian : Byteorder::BigEndian;
}

template<typename T>
void change_byteorder(T& t) {
	static_assert(std::is_arithmetic<T>::value, "change_byte_order<T> only works on primitive types");
	u8* buffer = reinterpret_cast<u8*>(&t);
	for(usize i = 0; i != sizeof(T) / 2; i++) {
		std::swap(buffer[i], buffer[sizeof(T) - (i + 1)]);
	}
}

template<Byteorder Order, typename T>
void set_byteorder(T& t) {
	if(Order != system_byteorder()) {
		change_byteorder(t);
	}
}

}
}

#endif // Y_IO_BYTEORDER_H
