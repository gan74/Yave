/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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
#include "BuffReader.h"

namespace y {
namespace io {

BuffReader::BuffReader(usize buff_size) : _size(buff_size), _offset(0), _used(0), _buffer(buff_size ? new u8[buff_size] : nullptr) {
}

BuffReader::BuffReader(ReaderRef&& r, usize buff_size) : BuffReader(buff_size) {
	_inner = std::move(r);
}

BuffReader::BuffReader(const ReaderRef& r, usize buff_size) : BuffReader(buff_size) {
	_inner = r;
}

BuffReader::~BuffReader() {
	delete[] _buffer;
}

BuffReader::BuffReader(BuffReader&& other) {
	swap(other);
}

BuffReader& BuffReader::operator=(BuffReader&& other) {
	swap(other);
	return *this;
}

void BuffReader::swap(BuffReader& other) {
	std::swap(_size, other._size);
	std::swap(_offset, other._offset);
	std::swap(_used, other._used);
	std::swap(_buffer, other._buffer);
	std::swap(_inner, other._inner);
}

bool BuffReader::at_end() const {
	return _inner->at_end() && !_used;
}

Reader::Result BuffReader::read(void* data, usize bytes) {
	return make_result([=]() {
		usize in_buffer = std::min(bytes, _used);

		std::memcpy(data, _buffer + _offset, in_buffer);
		_offset += in_buffer;
		_used -= in_buffer;

		if(usize remaining = bytes - in_buffer; remaining) {
			u8* data8 = reinterpret_cast<u8*>(data);
			if(remaining > _size) {
				return in_buffer + _inner->read(data8 + in_buffer, remaining).error_or(remaining);
			} else {
				_used = _inner->read(_buffer, _size).error_or(_size);
				_offset = 0;
				return  in_buffer + (_used ? read(data8 + in_buffer, remaining).error_or(remaining) : 0);
			}
		}
		return in_buffer;
	}(), bytes);
}

void BuffReader::read_all(core::Vector<u8>& data) {
	_inner->read_all(data);
	auto vec = core::Vector<u8>(_buffer + _offset, _buffer + _offset + _used);
	vec.push_back(data.begin(), data.end());
	data = std::move(vec);
	_used = 0;
}

}
}
