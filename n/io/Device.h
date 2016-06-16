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

#ifndef N_IO_DEVICE_H
#define N_IO_DEVICE_H

#include "InputStream.h"
#include "OutputStream.h"

namespace n {
namespace io {

class Device : public InputStream, public OutputStream
{
	public:
		enum OpenMode
		{
			None = 0x00,
			Read = 0x01,
			Write = 0x02,
			Binary = 0x04,
			AtEnd = 0x08,
			Append = Write | AtEnd
		};

		Device() {
		}

		virtual bool open(OpenMode) = 0;
		virtual void close() = 0;
		virtual OpenMode getOpenMode() const = 0;


		virtual bool isOpen() const {
			return getOpenMode() != None;
		}

		bool canWrite() const override {
			return getOpenMode() & Write;
		}

		bool canRead() const override {
			return !atEnd() && getOpenMode() & Read;
		}
};

inline Device::OpenMode operator|(Device::OpenMode a, Device::OpenMode b) {
	return Device::OpenMode(uint(a) | uint(b));
}

} // io
} // n

#endif // N_IO_IODEVICE_H
