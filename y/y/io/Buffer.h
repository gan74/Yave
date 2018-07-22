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
#ifndef Y_IO_BUFFER_H
#define Y_IO_BUFFER_H

#include <y/core/Vector.h>
#include <y/core/Result.h>

#include "Reader.h"
#include "Writer.h"
#include "Ref.h"

namespace y {
namespace io {

class Buffer final : public Reader, public Writer {
	public:
		Buffer(usize size = 0);

		bool at_end() const override;
		usize read(void* data, usize bytes) override;
		void read_all(core::Vector<u8>& data) override;

		usize remaining() const;

		void write(const void* data, usize bytes) override;
		void flush() override;

	private:
		core::Vector<u8> _buffer;
		usize _read_cursor = 0;

};

}
}

#endif // Y_IO_BUFFER_H
