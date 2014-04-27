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

#ifndef N_IO_FILE_H
#define N_IO_FILE_H

#include "IODevice.h"
#include <fstream>
#include <n/core/String.h>

namespace n {
namespace io {

class File : public IODevice
{
	public:
		File(const core::String &fileName);

		File(const File &) = delete;
		File &operator=(const File &) = delete;

		bool open(int m);
		void seek(uint pos);
		void close();
		bool isOpen() const;
		bool atEnd() const;
		bool canWrite() const;
		bool canRead() const;
		void flush();
		uint size() const;
		int getOpenMode() const;
		uint writeBytes(const char *b, uint len);
		uint readBytes(char *b, uint len = -1);


	private:
		core::String name;
		mutable std::fstream stream;
		int mode;
		uint length;
};

}// io
}// n

#endif // N_IO_FILE_H
