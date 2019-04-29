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
#ifndef Y_IO2_BUFFREADER_H
#define Y_IO2_BUFFREADER_H

#include "io.h"

namespace y {
namespace io2 {

template<typename T>
class BuffReader final : NonCopyable {

	public:
		static constexpr usize default_buffer_size = 4 * 1024;

		BuffReader(T&& w, usize buff_size = default_buffer_size) :
				_size(buff_size),
				_buffer(std::make_unique<u8[]>(_size)),
				_inner(std::move(w)) {
		}

		bool at_end() const {
			return !_buffered && _inner.at_end();
		}

		Result read(u8* data, usize max_bytes) {
			audit();
			const usize in_buffer = std::min(max_bytes, _buffered);

			// read from buffer first
			std::memcpy(data, _buffer.get() + _offset, in_buffer);
			_offset += in_buffer;
			_buffered -= in_buffer;
			audit();

			if(usize remaining = max_bytes - in_buffer; remaining) {
				// too big for buffer
				if(remaining > _size) {
					Result read = _inner.read(data + in_buffer, remaining);
					if(!read) {
						audit();
						return core::Err(read.error() + in_buffer);
					}
				} else {
					_buffered = _inner.read(_buffer.get(), _size).error_or(_size);
					_offset = std::min(remaining, _buffered);
					_buffered -= _offset;
					std::memcpy(data + in_buffer, _buffer.get(), _offset);
					audit();
					return check_len(_offset, remaining);
				}
			}
			audit();
			return core::Ok();
		}

		Result read_all(core::Vector<u8>& data) {
			audit();
			usize in_buffer = _buffered;
			data.push_back(_buffer.get() + _offset, _buffer.get() + _offset + _buffered);
			_buffered = 0;
			_offset = 0;
			Result res = _inner.read_all(data);
			audit();
			if(!res) {
				return core::Err(res.error() + in_buffer);
			}
			return core::Ok();
		}

	private:
		void audit() const {
			y_debug_assert(_buffer);
			y_debug_assert(_offset <= _size);
			y_debug_assert(_buffered <= _size);
			y_debug_assert(_buffered + _offset <= _size);
		}

		static Result check_len(usize len, usize expected) {
			if(len == expected) {
				return core::Ok();
			}
			return core::Err(len);
		}

		usize _size = 0;
		usize _offset = 0;
		usize _buffered = 0;
		std::unique_ptr<u8[]> _buffer = nullptr;

		T _inner;
};

}
}
#endif // Y_IO2_BUFFREADER_H
