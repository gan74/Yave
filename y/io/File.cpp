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
#include "File.h"


namespace y {
namespace io {

File::File() : _file(nullptr) {
}

File::File(FILE* f) : _file(f) {
}

File::~File() {
	if(_file) {
		fclose(_file);
	}
}

File::File(File&& other) : File() {
	swap(other);
}

File& File::operator=(File&& other) {
	swap(other);
	return *this;
}

void File::swap(File& other) {
	std::swap(_file, other._file);
}

File File::create(const core::String& name) {
	return fopen(name.data(), "wb+");
}

File File::open(const core::String& name) {
	return fopen(name.data(), "rb");
}

usize File::size() const {
	if(!_file) {
		return 0;
	}
	fpos_t pos = {};
	fgetpos(_file, &pos);
	fseek(_file, 0, SEEK_END);
	usize len = ftell(_file);
	fsetpos(_file, &pos);
	return len;
}

usize File::remaining() const {
	if(!_file) {
		return 0;
	}
	fpos_t pos = {};
	fgetpos(_file, &pos);
	usize offset = ftell(_file);
	fseek(_file, 0, SEEK_END);
	usize len = ftell(_file);
	fsetpos(_file, &pos);
	return len - offset;
}

bool File::at_end() const {
	return _file ? feof(_file) : true;
}

usize File::read(void* data, usize bytes) {
	if(!_file) {
		return 0;
	}
	return fread(data, 1, bytes, _file);
}

usize File::read_all(core::Vector<u8>& data) {
	usize left = remaining();
	data = core::Vector<u8>(left, 0);
	return read(data.begin(), left);
}

usize File::write(const void* data, usize bytes) {
	if(!_file) {
		return 0;
	}
	return fwrite(data, 1, bytes, _file);
}

void File::flush() {
	if(_file) {
		fflush(_file);
	}
}


}
}

