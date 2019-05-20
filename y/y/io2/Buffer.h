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
#ifndef Y_IO2_BUFFER_H
#define Y_IO2_BUFFER_H

#include "io.h"

namespace y {
namespace io2 {

class Buffer final : public Reader, public Writer {
	public:
		Buffer(usize size = 0) {
			_buffer.set_min_capacity(size);
		}

		bool at_end() const override {
			return _read_cursor >= _buffer.size();
		}

		usize remaining() const {
			return at_end() ? 0 : _buffer.size() - _read_cursor;
		}

		ReadResult read(u8* data, usize bytes) override {
			if(remaining() < bytes) {
				return core::Err<usize>(0);
			}
			std::copy_n(&_buffer[_read_cursor], bytes, data);
			_read_cursor += bytes;
			return core::Ok();
		}

		ReadUpToResult read_up_to(u8* data, usize max_bytes) override {
			usize max = std::min(max_bytes, remaining());
			std::copy_n(&_buffer[_read_cursor], max, data);
			_read_cursor += max;
			return core::Ok(max);
		}

		ReadUpToResult read_all(core::Vector<u8>& data) override {
			u8* start = &_buffer[_read_cursor];
			usize r = std::distance(start, _buffer.end());
			data.push_back(start, _buffer.end());
			_read_cursor += r;
			return core::Ok(r);
		}

		WriteResult write(const u8* data, usize bytes) override {
			_buffer.push_back(data, data + bytes);
			return core::Ok();
		}


		FlushResult flush() override {
			return core::Ok();
		}

	private:
		core::Vector<u8> _buffer;
		usize _read_cursor = 0;

};

}
}

#endif // Y_IO2_BUFFER_H
