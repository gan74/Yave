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

#ifndef N_IO_TEXTOUTPUTSTREAM_H
#define N_IO_TEXTOUTPUTSTREAM_H

#include "OutputStream.h"

namespace n {
namespace io {

class TextOutputStream : public OutputStream
{
	public:
		TextOutputStream(OutputStream *s) : stream(s) {
		}

		bool canWrite() const override {
			return stream->canWrite();
		}

		void flush() override {
			stream->flush();
		}

		uint writeBytes(const void *b, uint len) override {
			return stream->writeBytes(b, len);
		}

		template<typename T>
		uint write(const T &t) {
			core::String s(t);
			return stream->writeBytes(s.toChar(), s.size());
		}

		template<typename T>
		TextOutputStream &operator<<(const T &t) {
			write(t);
			return *this;
		}

	private:
		OutputStream *stream;
};

}
}

#endif // TEXTOUTPUTSTREAM_H
