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
#include <cstdio>

namespace n {
namespace io {

bool File::exists(const core::String &fileName) {
	FILE *file = fopen(fileName.toChar(), "r");
	if(!file) {
		return false;
	}
	fclose(file);
	return true;
}

File::File(const core::String &fileName) : IODevice(), file(0), name(fileName), mode(IODevice::None) {
}

const core::String &File::getName() const {
	return name;
}

core::String File::getPath() const {
	core::String pname = name.replaced("\\", "/").replaced("//", "/");
	if(pname.endsWith("/")) {
		return pname;
	}
	uint it = pname.find("/");
	uint last = it;
	if(it == uint(-1)) {
		return ".";
	}
	while(it != uint(-1)) {
		last = it;
		it = pname.find("/", last + 1);
	}
	return pname.subString(0, last);
}

bool File::exists() const {
	return exists(getName());
}

bool File::remove() {
	return !::remove(name.toChar());
}

bool File::open(OpenMode m) {
	if(m == IODevice::None) {
		close();
		return false;
	}
	if(isOpen()) {
		close();
	}
	core::String openMode = "r";
	if(m & IODevice::Write) {
		openMode = "w";
		if(m & IODevice::Read) {
			openMode += "+";
		}
	} else if(m & IODevice::AtEnd) {
		openMode = "a";
		if(m & IODevice::Read) {
			openMode += "+";
		}
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
}

void File::seek(uint pos) {
	if(isOpen()) {
		fseek(file, pos, SEEK_SET);
	}
}

uint File::getPos() const {
	if(isOpen()) {
		return ftell(file);
	}
	return 0;
}

void File::close() {
	if(file) {
		fclose(file);
		file = 0;
		mode = IODevice::None;
	}
}

bool File::atEnd() const {
	return !isOpen() || getPos() >= length;
}

void File::flush() {
	if(isOpen()) {
		fflush(file);
	}
}

uint File::size() const {
	if(!isOpen()) {
		return 0;
	}
	return length;
}

IODevice::OpenMode File::getOpenMode() const {
	return mode;
}

uint File::writeBytes(const void *b, uint len) {
	if(!canWrite()) {
		return 0;
	}
	length = std::max(getPos() + len, length);
	return fwrite(b, sizeof(char), len, file);
}

uint File::readBytes(void *b, uint len) {
	if(!canRead()) {
		return 0;
	}
	uint l = std::min(len, length - getPos());
	return fread(b, sizeof(char), l, file);
}

}// io
}// n
