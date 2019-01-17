/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include "BuffWriter.h"

namespace y {
namespace io {

static void error() {
	y_throw("Unable to write.");
}


BuffWriter::BuffWriter(usize buff_size) : _size(buff_size), _buffer(buff_size ? new u8[buff_size] : nullptr) {
}

BuffWriter::BuffWriter(WriterRef&& w, usize buff_size) : BuffWriter(buff_size) {
	_inner = std::move(w);
}

BuffWriter::BuffWriter(const WriterRef& w, usize buff_size) : BuffWriter(buff_size) {
	_inner = w;
}

BuffWriter::~BuffWriter() {
	flush();
	delete[] _buffer;
}

BuffWriter::BuffWriter(BuffWriter&& other) {
	swap(other);
}

BuffWriter& BuffWriter::operator=(BuffWriter&& other) {
	swap(other);
	return *this;
}

void BuffWriter::swap(BuffWriter& other) {
	std::swap(_size, other._size);
	std::swap(_used, other._used);
	std::swap(_buffer, other._buffer);
}

void BuffWriter::write(const void* data, usize bytes) {
	if(!bytes) {
		return;
	}

	usize previous = _used;
	usize free = _size - _used;

	if(bytes >= free + _size) {
		usize flushed = flush_r();
		if(flushed == previous) {
			_inner->write(data, bytes);
			return;
		}
		error();
	}

	usize first = std::min(bytes, free);
	std::memcpy(_buffer + _used, data, first);
	_used += first;
	if(_used == _size) {
		usize flushed = flush_r();
		if(flushed == _size) {
			std::memcpy(_buffer, reinterpret_cast<const u8*>(data) + first, _used = (bytes - first));
			return;
		} else if(flushed < previous) {
			error();
		}
		error();
	}
}

usize BuffWriter::flush_r() {
	if(_used) {
		_inner->write(_buffer, _used);
		usize writen = _used;
		_used = 0;
		return writen;
	}
	return 0;
}

void BuffWriter::flush() {
	flush_r();
}


}
}
