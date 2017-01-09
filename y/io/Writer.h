/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#ifndef Y_IO_WRITER_H
#define Y_IO_WRITER_H

#include <y/utils.h>
#include <y/core/Vector.h>

namespace y {
namespace io {

class Writer : NonCopyable {
	public:
		virtual ~Writer();

		virtual usize write(const void* data, usize bytes) = 0;
		virtual void flush() = 0;

		template<typename T>
		usize write(const T* data, usize size) {
			return write(reinterpret_cast<const void*>(data), size * sizeof(T)) / sizeof(T);
		}

		template<typename T>
		usize write_vec(const core::Vector<T>& data) {
			return write(data.begin(), data.size());
		}

};

}
}

#endif // Y_IO_WRITER_H
