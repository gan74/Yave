/*******************************
Copyright (C) 2009-2010 grégoire ANGERAND

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

#ifndef N_IO_INPUTSTREAM_H
#define N_IO_INPUTSTREAM_H

#include <n/Types.h>

namespace n {
namespace core {
class String;
}
}

namespace n {
namespace io {

class InputStream
{
	public:
		virtual bool atEnd() const = 0;
		virtual bool canRead() const = 0;
		virtual uint readBytes(char *b, uint l = -1) = 0;

};


} // io
} // n

#endif // N_IO_INPUTSTREAM_H
