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
#ifndef Y_IO_IO_H
#define Y_IO_IO_H

#include <y/utils.h>
#include <y/core/Vector.h>
#include <y/core/String.h>
#include <y/math/Vec.h>

#include <istream>

namespace y {
namespace io {

template<typename T>
void change_byte_order(T &t) {
	static_assert(std::is_arithmetic<T>::value, "change_byte_order<T> only works on primitive types");
	char *buffer = reinterpret_cast<char *>(&t);
	for(usize i = 0; i != sizeof(T) / 2; i++) {
		std::swap(buffer[i], buffer[sizeof(T) - (i + 1)]);
	}
}

template<typename T, typename Enable = typename std::enable_if<std::is_arithmetic<T>::value>::type>
bool decode(std::istream &stream, T &t) {
	stream.read(reinterpret_cast<char *>(&t), sizeof(T));
	if(!stream.good()) {
		return false;
	}
	if(is_little_endian()) {
		change_byte_order(t);
	}
	return true;
}

template<typename T, typename RP>
bool decode(std::istream &stream, core::Vector<T, RP> &vec) {
	u64 len = 0;
	Y_TRY(decode(stream, len));
	vec = core::Vector<T>(len, T());
	for(u64 i = 0; i != len; i++) {
		Y_TRY(decode(stream, vec[i]));
	}
	return true;
}

template<y::usize N, typename T>
bool decode(std::istream &stream, math::Vec<N, T> &vec) {
	for(usize i = 0; i != N; i++) {
		Y_TRY(decode(stream, vec[i]));
	}
	return true;
}

}
}

#endif // Y_IO_IO_H
