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
#ifndef N_IO_BUFFER_H
#define N_IO_BUFFER_H

#include "Device.h"
#include <n/core/Array.h>

namespace n {
namespace io {

class Buffer : public Device
{
	public:
		Buffer();

		virtual bool open(OpenMode m) override;
		virtual void close() override;
		virtual OpenMode getOpenMode() const override;

		virtual bool atEnd() const override;
		virtual uint readBytes(void *b, uint l = -1) override;

		virtual uint writeBytes(const void *b, uint len) override;
		virtual void flush() override;

	private:
		core::Array<byte> buffer;
		uint iterator;
		OpenMode mode;
};


}
}

#endif // BUFFER_H
