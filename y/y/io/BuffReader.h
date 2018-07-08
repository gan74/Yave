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
#ifndef Y_IO_BUFFREADER_H
#define Y_IO_BUFFREADER_H

#include "Ref.h"

namespace y {
namespace io {

class BuffReader final : public Reader {

	public:
		static constexpr usize default_buffer_size = 4 * 1024;

		BuffReader(ReaderRef&& r, usize buff_size = default_buffer_size);
		BuffReader(const ReaderRef& r, usize buff_size = default_buffer_size);

		~BuffReader() override;

		BuffReader(BuffReader&& other);
		BuffReader& operator=(BuffReader&& other);

		bool at_end() const override;

		usize read(void *data, usize bytes) override;
		void read_all(core::Vector<u8>& data) override;

	private:
		BuffReader(usize buff_size);

		void swap(BuffReader& other);

		usize _size = 0;
		usize _offset = 0;
		usize _used = 0;
		u8* _buffer = nullptr;

		ReaderRef _inner;
};

}
}
#endif // Y_IO_BUFFREADER_H
