/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
namespace io2 {

static WriteResult check_len_w(usize len, usize expected) {
	if(len == expected) {
		return core::Ok();
	}
	return core::Err(len);
}


File::File(std::FILE* f) : _file(f) {
}

File::~File() {
	if(_file) {
		std::fclose(_file);
	}
}

File::File(File&& other) {
	swap(other);
}

File& File::operator=(File&& other) {
	swap(other);
	return *this;
}

void File::swap(File& other) {
	std::swap(_file, other._file);
}

core::Result<File> File::create(const core::String& name) {
	std::FILE* file = std::fopen(name.begin(), "wb+");
	if(file) {
		return core::Ok<File>(file);
	}
	return core::Err();
}

core::Result<File> File::open(const core::String& name) {
	std::FILE* file = std::fopen(name.begin(), "rb");
	if(file) {
		return core::Ok<File>(file);
	}
	return core::Err();
}

core::Result<void> File::copy(Reader& src, const core::String& dst) {
	auto f = create(dst);
	if(!f) {
		return core::Err();
	}

	File dst_file = std::move(f.unwrap());
	u8 buffer[1024];
	while(!src.at_end()) {
		if(auto r = src.read_up_to(buffer, sizeof(buffer))) {
			if(dst_file.write(buffer, r.unwrap())) {
				continue;
			}
		}
		return core::Err();
	}
	return core::Ok();
}

usize File::size() const {
	if(!_file) {
		return 0;
	}
	std::fpos_t pos = {};
	std::fgetpos(_file, &pos);
	std::fseek(_file, 0, SEEK_END);
	auto len = usize(std::ftell(_file));
	std::fsetpos(_file, &pos);
	return len;
}

usize File::remaining() const {
	if(!_file) {
		return 0;
	}
	std::fpos_t pos = {};
	std::fgetpos(_file, &pos);
	auto offset = usize(std::ftell(_file));
	std::fseek(_file, 0, SEEK_END);
	auto len = usize(std::ftell(_file));
	std::fsetpos(_file, &pos);
	return len - offset;
}

bool File::is_open() const {
	return _file;
}

bool File::at_end() const {
	return _file ? (std::feof(_file) || !remaining()) : true;
}

void File::seek(usize byte) {
	if(_file) {
		std::fseek(_file, byte, SEEK_SET);
	}
}

ReadResult File::read(u8* data, usize bytes) {
	if(!_file) {
		return core::Err<usize>(0);
	}
	usize r = std::fread(data, 1, bytes, _file);
	if(r != bytes) {
		return core::Err(r);
	}
	return core::Ok();
}

ReadUpToResult File::read_up_to(u8* data, usize max_bytes) {
	if(!_file) {
		return core::Err<usize>(0);
	}
	usize r = std::fread(data, 1, max_bytes, _file);
	if(std::ferror(_file)) {
		return core::Err(r);
	}
	return core::Ok(r);
}

ReadUpToResult File::read_all(core::Vector<u8>& data) {
	usize left = remaining();
	usize size = data.size();
	data.set_min_capacity(left + size);
	std::fill_n(std::back_inserter(data), left, 0);
	return read_up_to(data.begin() + size, left);
}

WriteResult File::write(const u8* data, usize bytes) {
	if(!_file) {
		return core::Err<usize>(0);
	}
	return check_len_w(std::fwrite(data, 1, bytes, _file), bytes);
}

FlushResult File::flush() {
	if(_file) {
		std::fflush(_file);
		return core::Ok();
	}
	return core::Err();
}


}
}

