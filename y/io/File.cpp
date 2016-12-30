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
	auto len = usize(ftell(_file));
	fsetpos(_file, &pos);
	return len;
}

usize File::remaining() const {
	if(!_file) {
		return 0;
	}
	fpos_t pos = {};
	fgetpos(_file, &pos);
	auto offset = usize(ftell(_file));
	fseek(_file, 0, SEEK_END);
	auto len = usize(ftell(_file));
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

