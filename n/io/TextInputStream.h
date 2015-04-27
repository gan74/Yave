/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

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

#ifndef N_IO_TEXTINPUTSTREAM_H
#define N_IO_TEXTINPUTSTREAM_H

#include "InputStream.h"
#include <n/core/String.h>

namespace n {
namespace io {

class TextInputStream : public InputStream
{
	public:
		TextInputStream(InputStream *s) : stream(s) {
		}

		bool canRead() const override {
			return stream->canRead();
		}

		uint readBytes(char *b, uint len = -1) override {
			return stream->readBytes(b, len);
		}

		bool atEnd() const override {
			return stream->atEnd();
		}

		core::String readLine() {
			core::Array<char> arr;
			char c;
			while(true) {
				readBytes(&c, 1);
				if(c != '\n') {
					arr.append(c);
				} else {
					break;
				}
			}
			return core::String(arr);
		}

		core::String readBlock() {
			core::Array<char> arr;
			char c;
			while(true) {
				readBytes(&c, 1);
				if(!isspace(c)) {
					arr.append(c);
				} else {
					break;
				}
			}
			return core::String(arr);
		}

		template<typename T>
		T read() {
			return T(readBlock());
		}

		template<typename T>
		TextInputStream &operator>>(T &t) {
			t = T(readBlock());
			return *this;
		}

	private:
		InputStream *stream;
};
}
}

#endif // TextInputStream_H
