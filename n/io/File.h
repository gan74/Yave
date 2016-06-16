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

#ifndef N_IO_FILE_H
#define N_IO_FILE_H

#include "Device.h"
#include <n/core/String.h>

namespace n {
namespace io {

class File : public Device
{
	public:
		File(const core::String &fileName);

		bool open(OpenMode m) override;
		void seek(uint pos);
		uint getPos() const;
		void close() override;
		bool atEnd() const override;
		void flush() override;
		uint size() const;
		OpenMode getOpenMode() const override;
		uint writeBytes(const void *b, uint len) override;
		uint readBytes(void *b, uint len = -1) override;
		bool exists() const;
		bool remove();

		const core::String &getName() const;
		core::String getPath() const;

		static bool exists(const core::String &fileName);

	private:
		FILE *file;

		core::String name;
		OpenMode mode;
		uint length;
};

}// io
}// n

#endif // N_IO_FILE_H
