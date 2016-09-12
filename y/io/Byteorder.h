/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
