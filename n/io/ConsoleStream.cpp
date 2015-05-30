/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

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

#include "ConsoleStream.h"


namespace n {
namespace io {

ConsoleInputStream::ConsoleInputStream() : TextInputStream(this) {
}

bool ConsoleInputStream::atEnd() const {
	return true;
}

bool ConsoleInputStream::canRead() const {
	return true;
}

uint ConsoleInputStream::readBytes(void *b, uint l) {
	return fread(b, sizeof(char), l, stdin);
}

ConsoleOutputStream::ConsoleOutputStream(Channel ch) : TextOutputStream(this), channel(ch) {
}

bool ConsoleOutputStream::canWrite() const {
	return true;
}

uint ConsoleOutputStream::writeBytes(const void *b, uint len) {
	fwrite(b, sizeof(char), len, channel == ConsoleOutputStream::Out ? stdout : stderr);
	return len;
}

void ConsoleOutputStream::flush() {
	fflush(channel == ConsoleOutputStream::Out ? stdout : stderr);
}

ConsoleInputStream in = ConsoleInputStream();
ConsoleOutputStream out = ConsoleOutputStream(ConsoleOutputStream::Out);
ConsoleOutputStream err = ConsoleOutputStream(ConsoleOutputStream::Error);

}
}
