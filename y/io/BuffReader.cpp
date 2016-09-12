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

BuffReader::BuffReader(BuffReader&& other) : BuffReader() {
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

usize BuffReader::read(void* data, usize bytes) {
	usize in_buffer = std::min(bytes, _used);

	memcpy(data, _buffer + _offset, in_buffer);
	_offset += in_buffer;
	_used -= in_buffer;

	usize remaining = bytes - in_buffer;
	if(remaining) {
		u8* data8 = reinterpret_cast<u8*>(data);
		if(remaining > _size) {
			return in_buffer + _inner->read(data8 + in_buffer, remaining);
		} else {
			_used = _inner->read(_buffer, _size);
			_offset = 0;
			return  in_buffer + (_used ? read(data8 + in_buffer, remaining) : 0);
		}
	}
	return in_buffer;
}

usize BuffReader::read_all(core::Vector<u8>& data) {
	usize l = _inner->read_all(data);
	auto vec = core::Vector<u8>(_used, 0);
	memcpy(vec.begin(), _buffer + _offset, _used);
	vec << core::range(data);
	data = vec;
	l += _used;
	_used = 0;
	return l;
}

}
}
