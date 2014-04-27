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

#include "File.h"

namespace n {
namespace io {

File::File(const core::String &fileName) : IODevice(), name(fileName), mode(IODevice::None) {
}

bool File::open(int m) {
	if(m == IODevice::None) {
		close();
		return false;
	} else {
		if(isOpen()) {
			close();
		}
		std::ios_base::openmode om = std::fstream::in;
		if(m & IODevice::Write) {
			if(m & IODevice::Read) {
				om |= std::fstream::out;
			} else {
				om = std::fstream::out;
			}
		}
		if(m & IODevice::Binary) {
			om |= std::fstream::binary;
		}
		stream.open(name.toChar(), om);
		if(!stream.is_open()) {
			mode = IODevice::None;
			return false;
		}
		stream.seekg(0, std::ios::end);
		length = stream.tellg();
		stream.seekg(0, std::ios::beg);
		mode = m;
		return true;
	}
}

void File::seek(uint pos) {
	if(isOpen()) {
		stream.seekg(pos, std::ios::beg);
		stream.seekp(pos, std::ios::beg);
	}
}

void File::close() {
	stream.close();
	mode = IODevice::None;
}

bool File::isOpen() const {
	return mode != IODevice::None;
}

bool File::atEnd() const {
	return stream.tellg() == length;
}

bool File::canWrite() const {
	return mode & IODevice::Write;
}

bool File::canRead() const {
	return !atEnd() && mode & IODevice::Read;
}

void File::flush() {
	if(isOpen()) {
		stream.flush();
	}
}

uint File::size() const {
	if(!isOpen()) {
		return 0;
	}
	return length;
}

int File::getOpenMode() const {
	return mode;
}

uint File::writeBytes(const char *b, uint len) {
	if(!canWrite()) {
		return 0;
	}
	if(atEnd()) {
		length += len;
	}
	stream.write(b, len);
	return len;
}

uint File::readBytes(char *b, uint len) {
	if(!canRead()) {
		return 0;
	}
	uint l = std::min(len, length - (uint)stream.tellg());
	stream.read(b, l);
	/*int ch = stream.get();
	while(ch != std::istream::traits_type::eof()) {
		*(b++) = ch;
		ch = stream.get();
	}*/
	return l;
}

}// io
}// n
