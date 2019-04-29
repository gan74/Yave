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
#ifndef Y_IO2_BUFFWRITER_H
#define Y_IO2_BUFFWRITER_H

#include "io.h"

namespace y {
namespace io2 {

template<typename T>
class BuffWriter final : NonCopyable {

	public:
		static constexpr usize default_buffer_size = 4 * 1024;

		BuffWriter(T&& w, usize buff_size = default_buffer_size) :
				_size(buff_size),
				_buffer(std::make_unique<u8[]>(buffer_size)),
				_inner(std::move(w)) {
		}

		~BuffWriter() {
			flush();
		}

		BuffWriter(BuffWriter&&) = default;
		BuffWriter& operator=(BuffWriter&&) = default;

		Result<usize> write(const u8* data, usize bytes) {
			if(!bytes) {
				return core::Ok();
			}
			usize previous = _used;
			usize free = _size - _used;

			auto written = [&] {
					// can not fit in buffer
					if(bytes >= free + _size) {
						Result flushed = flush_r();
						if(flushed && flushed.unwrap() == previous) {
							return _inner.write(data, bytes);
						}
						// fail to flush previous data: 0 byte written for this one
						return flushed;
					}

					// write whatever we can in buffer
					usize first = std::min(bytes, free);
					std::memcpy(_buffer + _used, data, first);
					_used += first;

					// flush and write the rest
					if(_used == _size) {
						Result flushed = flush_r();
						if(!flushed) {
							return flushed;
						}
						if(flushed.unwrap()) {
							std::memcpy(_buffer, data + first, _used = (bytes - first));
							return core::Ok();
						}
					}
					return core::Ok();
				}();

			if(written) {
				return written;
			}
			return core::Err(std::max(previous, written.error()) - previous);
		}

		void flush() {
			flush_r();
		}

	private:
		Result flush_r() {
			if(_used) {
				Result flushed = _inner.write(_buffer, _used);
				if(flushed.is_error()) {
					return flushed;
				}
				_used = 0;
			}
			return core::Ok();
		}

		usize _size = 0;
		usize _used = 0;
		std::unique_ptr<u8[]> _buffer = nullptr;

		T _inner;
};

}
}

#endif // Y_IO2_BUFFWRITER_H
