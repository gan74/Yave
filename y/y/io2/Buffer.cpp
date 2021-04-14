/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
namespace io2 {

Buffer::Buffer(usize size) {
    _buffer.set_min_capacity(size);
}

Buffer::~Buffer() {
}

bool Buffer::at_end() const {
    y_debug_assert(_cursor <= _buffer.size());
    return _cursor == _buffer.size();
}

usize Buffer::remaining() const {
    y_debug_assert(_cursor <= _buffer.size());
    return at_end() ? 0 : _buffer.size() - _cursor;
}

void Buffer::seek(usize byte) {
    _cursor = std::min(_buffer.size(), byte);
    y_debug_assert(_cursor <= _buffer.size());
}

void Buffer::seek_end() {
    _cursor = _buffer.size();
}

void Buffer::reset() {
    _cursor = 0;
}

void Buffer::clear() {
    _cursor = 0;
    _buffer.make_empty();
}

usize Buffer::tell() const {
    return _cursor;
}

ReadResult Buffer::read(void* data, usize bytes) {
    if(remaining() < bytes) {
        return core::Err<usize>(0);
    }
    y_debug_assert(_cursor < _buffer.size());
    std::copy_n(&_buffer[_cursor], bytes, static_cast<u8*>(data));
    _cursor += bytes;
    return core::Ok();
}

ReadUpToResult Buffer::read_up_to(void* data, usize max_bytes) {
    const usize max = std::min(max_bytes, remaining());
    y_debug_assert(_cursor < _buffer.size() || !max);
    std::copy_n(&_buffer[_cursor], max, static_cast<u8*>(data));
    _cursor += max;
    y_debug_assert(_cursor <= _buffer.size());
    return core::Ok(max);
}

ReadUpToResult Buffer::read_all(core::Vector<u8>& data) {
    u8* start = _buffer.data() + _cursor;
    const usize r = std::distance(start, _buffer.end());
    data.push_back(start, _buffer.end());
    _cursor += r;
    y_debug_assert(_cursor <= _buffer.size());
    return core::Ok(r);
}

WriteResult Buffer::write(const void* data, usize bytes) {
    const u8* data_bytes = static_cast<const u8*>(data);
    if(at_end()) {
        _buffer.push_back(data_bytes, data_bytes + bytes);
        _cursor += bytes;
        y_debug_assert(_cursor <= _buffer.size());
    } else {
        const usize end = std::min(_buffer.size(), _cursor + bytes);
        const usize overwrite = end - _cursor;
        y_debug_assert(overwrite <= bytes);
        std::copy_n(data_bytes, overwrite, &_buffer[_cursor]);
        _buffer.push_back(data_bytes + overwrite, data_bytes + bytes);
        _cursor += bytes - overwrite;
        y_debug_assert(_cursor <= _buffer.size());
    }
    return core::Ok();
}


FlushResult Buffer::flush() {
    return core::Ok();
}

const u8* Buffer::data() const {
    return _buffer.data();
}

usize Buffer::size() const {
    return _buffer.size();
}

}
}

