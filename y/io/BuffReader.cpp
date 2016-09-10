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


BuffReader::BuffReader() : buffer_size(0), buffer_offset(0), buffer_used(0), buffer(nullptr), inner(nullptr) {
}

BuffReader::BuffReader(Read &r) : buffer_size(512), buffer_offset(0), buffer_used(0), buffer(new u8[buffer_size]), inner(&r) {
}

BuffReader::~BuffReader() {
	delete[] buffer;
}

BuffReader::BuffReader(BuffReader &&other) : BuffReader() {
	swap(other);
}

BuffReader &BuffReader::operator=(BuffReader &&other) {
	swap(other);
	return *this;
}

void BuffReader::swap(BuffReader &other) {
	std::swap(buffer_size, other.buffer_size);
	std::swap(buffer_offset, other.buffer_offset);
	std::swap(buffer_used, other.buffer_used);
	std::swap(buffer, other.buffer);
	std::swap(inner, other.inner);
}

usize BuffReader::read(void *data, usize bytes) {
	usize in_buffer = std::min(bytes, buffer_used);

	memcpy(data, buffer + buffer_offset, in_buffer);
	buffer_offset += in_buffer;
	buffer_used -= in_buffer;

	usize remaining = bytes - in_buffer;
	if(remaining) {
		u8 *data8 = reinterpret_cast<u8 *>(data);
		if(remaining > buffer_size) {
			return in_buffer + inner->read(data8 + in_buffer, remaining);
		} else {
			inner->read(buffer, buffer_size);
			buffer_offset = buffer_used = 0;
			return in_buffer + read(data8 + in_buffer, remaining);
		}
	}
	return in_buffer;
}

usize BuffReader::read_all(core::Vector<u8> &data) {
	usize l = inner->read_all(data);
	auto vec = core::Vector<u8>(buffer_used, 0);
	memcpy(vec.begin(), buffer + buffer_offset, buffer_used);
	vec << core::range(data);
	data = vec;
	l += buffer_used;
	buffer_used = 0;
	return l;
}

}
}
