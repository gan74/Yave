/*******************************
Copyright (C) 2013-2014 grégoire ANGERAND

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

#include "InputStream.h"
#include <n/core/String.h>
#include <n/core/Array.h>

namespace n {
namespace io {

DataInputStream::DataInputStream(InputStream *s) : stream(s) {
}

bool DataInputStream::canRead() const {
	return stream->canRead();
}

uint DataInputStream::readBytes(char *b, uint len) {
	return stream->readBytes(b, len);
}

core::String DataInputStream::readLine() {
	core::Array<char> arr(6);
	char c;
	while(true) {
		readBytes(&c, 1);
		if(c != '\n') {
			arr.append(c);
		} else {
			break;
		}
	}
	arr.append('\0');
	return core::String(arr.begin(), arr.size());
}

int DataInputStream::readInt() {
	core::String s = readLine();
	return atoi(s.toChar());
}

double DataInputStream::readDouble() {
	core::String s = readLine();
	return atof(s.toChar());
}


} // io
} // n
