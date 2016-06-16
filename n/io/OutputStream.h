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

#ifndef N_IO_OUTPUTSTREAM_H
#define N_IO_OUTPUTSTREAM_H

#include <n/core/String.h>

namespace n {
namespace io {

class OutputStream : NonCopyable
{
	public:
		virtual ~OutputStream() {}

		virtual bool canWrite() const = 0;
		virtual uint writeBytes(const void *b, uint len) = 0;
		virtual void flush() = 0;

		/*virtual void write(const core::String &str) {
			if(str.size()) {
				writeBytes(str.data(), str.size());
			}
		}*/
};


} // io
} // n

#endif // N_IO_OUTPUTSTREAM_H
