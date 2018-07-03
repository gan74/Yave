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
#ifndef Y_IO_BUFFWRITER_H
#define Y_IO_BUFFWRITER_H

#include "Ref.h"

namespace y {
namespace io {

class BuffWriter final : public Writer {

	public:
		static constexpr usize default_buffer_size = 4 * 1024;

		BuffWriter(WriterRef&& w, usize buff_size = default_buffer_size);
		BuffWriter(const WriterRef& w, usize buff_size = default_buffer_size);

		~BuffWriter() override;

		BuffWriter(BuffWriter&& other);
		BuffWriter& operator=(BuffWriter&& other);

		Result write(const void* data, usize bytes) override;
		void flush() override;

	private:
		BuffWriter(usize buff_size = 0);

		usize flush_r();

		void swap(BuffWriter& other);

		usize _size = 0;
		usize _used = 0;
		u8* _buffer = nullptr;

		WriterRef _inner;
};

}
}

#endif // Y_IO_BUFFWRITER_H
