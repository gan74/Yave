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
		File(const core::String &fileName) : IODevice(), name(fileName), mode(IODevice::None) {
		}

		File(const File &) = delete;
		File &operator=(const File &) = delete;

		bool open(int m) {
			if(m == IODevice::None) {
				close();
				return false;
			} else {
				if(isOpen()) {
					close();
				}
				std::ios_base::openmode om = std::fstream::in;
				if(m & IODevice::Write) {
					if(m & IODevice::Read) {
						om |= std::fstream::out;
					} else {
						om = std::fstream::out;
					}
				}
				if(m & IODevice::Binary) {
					om |= std::fstream::binary;
				}
				stream.open(name.toChar(), om);
				if(!stream.is_open()) {
					mode = IODevice::None;
					return false;
				}
				stream.seekg(0, std::ios::end);
				length = stream.tellg();
				stream.seekg(0, std::ios::beg);
				mode = m;
				return true;
			}
		}

		void seek(uint pos) {
			if(isOpen()) {
				stream.seekg(pos, std::ios::beg);
				stream.seekp(pos, std::ios::beg);
			}
		}

		void close() {
			stream.close();
			mode = IODevice::None;
		}

		bool isOpen() const {
			return mode != IODevice::None;
		}

		bool atEnd() const {
			return stream.tellg() == length;
		}

		bool canWrite() const {
			return mode & IODevice::Write;
		}

		bool canRead() const {
			return !atEnd() && mode & IODevice::Read;
		}

		void flush() {
			if(isOpen()) {
				stream.flush();
			}
		}

		uint size() const {
			if(!isOpen()) {
				return 0;
			}
			return length;
		}

		int getOpenMode() const {
			return mode;
		}

		uint writeBytes(const char *b, uint len) {
			if(!canWrite()) {
				return 0;
			}
			if(atEnd()) {
				length += len;
			}
			stream.write(b, len);
			return len;
		}

		uint readBytes(char *b, uint len = -1) {
			if(!canRead()) {
				return 0;
			}
			uint l = std::min(len, length - (uint)stream.tellg());
			stream.read(b, l);
			/*int ch = stream.get();
			while(ch != std::istream::traits_type::eof()) {
				*(b++) = ch;
				ch = stream.get();
			}*/
			return l;
		}



	private:
		core::String name;
		int mode;
		mutable std::fstream stream;
		uint length;
};

}// io
}// n

#endif // N_IO_FILE_H
