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

#ifndef N_IO_IODEVICE_H
#define N_IO_IODEVICE_H

#include "InputStream.h"
#include "OutputStream.h"

namespace n {
namespace io {

class IODevice : public InputStream, public OutputStream
{
	public:
		static const int None = 0x00;
		static const int Read = 0x01;
		static const int Write = 0x02;
		static const int Binary = 0x04;
		static const int Append = 0x08;


		IODevice() {
		}

		virtual bool open(int) = 0;
		virtual void close() = 0;
		virtual bool isOpen() const = 0;
		virtual int getOpenMode() const = 0;
};

} // io
} // n

#endif // N_IO_IODEVICE_H
