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

#include "File.h"

namespace n {
namespace io {

File::File(const core::String &fileName) : IODevice(),
#ifdef N_IO_USE_C_FILE
	file(0),
#endif
	name(fileName), mode(IODevice::None) {
}

const core::String &File::getName() const {
	return name;
}

bool File::open(int m) {
	#ifndef N_IO_USE_C_FILE
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
		/*if(m & IODevice::Binary)*/ {
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
	#else
	if(m == IODevice::None) {
		close();
		return false;
	}
	if(isOpen()) {
		close();
	}
	core::String openMode = "r";
	if(m & IODevice::Read && m & IODevice::Write) {
		openMode = "w+";
	} else if(m & IODevice::Write) {
		openMode = "w";
	}
	if(m & IODevice::Binary)	{
		openMode += "b";
	}
	file = fopen(name.toChar(), openMode.toChar());
	if(!file) {
		return false;
	}
	mode = m;
	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);
	return true;
	#endif
}

void File::seek(uint pos) {
	#ifndef N_IO_USE_C_FILE
	if(isOpen()) {
		stream.seekg(pos, std::ios::beg);
		stream.seekp(pos, std::ios::beg);
	}
	#else
	if(isOpen()) {
		fseek(file, pos, SEEK_SET);
	}
	#endif
}

uint File::getPos() const {
	if(isOpen()) {
		#ifndef N_IO_USE_C_FILE
			return stream.tellg();
		#else
			return ftell(file);
		#endif
	}
	return 0;
}

void File::close() {
	#ifndef N_IO_USE_C_FILE
	stream.close();
	mode = IODevice::None;
	#else
	if(file) {
		fclose(file);
		file = 0;
		mode = IODevice::None;
	}
	#endif
}

bool File::isOpen() const {
	return mode != IODevice::None;
}

bool File::atEnd() const {
	return !isOpen() || getPos() >= length;
}

bool File::canWrite() const {
	return mode & IODevice::Write;
}

bool File::canRead() const {
	return !atEnd() && mode & IODevice::Read;
}

void File::flush() {
	#ifndef N_IO_USE_C_FILE
	if(isOpen()) {
		stream.flush();
	}
	#else
	if(isOpen()) {
		fflush(file);
	}
	#endif
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

uint File::writeBytes(const void *b, uint len) {
	if(!canWrite()) {
		return 0;
	}
	length = std::max(getPos() + len, length);
	#ifndef N_IO_USE_C_FILE
	stream.write(b, len);
	return len;
	#else
	return fwrite(b, sizeof(char), len, file);
	#endif
}

uint File::readBytes(void *b, uint len) {
	if(!canRead()) {
		return 0;
	}
	uint l = std::min(len, length - getPos());
	#ifndef N_IO_USE_C_FILE
	stream.read(b, l);
	/*int ch = stream.get();
	while(ch != std::istream::traits_type::eof()) {
		*(b++) = ch;
		ch = stream.get();
	}*/
	return l;
	#else
	return fread(b, sizeof(char), l, file);
	#endif
}

}// io
}// n
