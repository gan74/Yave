/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include "Buffer.h"

namespace y {
namespace io {

Buffer::Buffer(usize size) {
	_buffer.set_min_capacity(size);
}

bool Buffer::at_end() const {
	return _read_cursor >= _buffer.size();
}

usize Buffer::read(void* data, usize bytes) {
	if(at_end()) {
		return 0;
	}
	usize max = std::min(bytes, remaining());
	std::copy_n(&_buffer[_read_cursor], max, reinterpret_cast<u8*>(data));

	_read_cursor += max;
	return max;
}

void Buffer::read_all(core::Vector<u8>& data) {
	data.assign(_buffer.begin(), _buffer.end());
}

usize Buffer::remaining() const {
	return at_end() ? 0 : _buffer.size() - _read_cursor;
}

void Buffer::write(const void* data, usize bytes) {
	const u8* dat = reinterpret_cast<const u8*>(data);
	_buffer.push_back(dat, dat + bytes);
}

void Buffer::flush() {
}

}
}
