/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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
#ifndef Y_IO_FILE_H
#define Y_IO_FILE_H

#include <y/core/String.h>

#include "Reader.h"
#include "Writer.h"

namespace y {
namespace io {

class File : public Reader, public Writer {

	Y_TODO(use exceptions for file)

	public:
		File();
		virtual ~File();

		File(File&& other);
		File& operator=(File&& other);

		static File create(const core::String& name);
		static File open(const core::String& name);

		usize size() const;
		usize remaining() const;

		virtual bool at_end() const override;

		virtual usize read(void* data, usize bytes) override;
		virtual usize read_all(core::Vector<u8>& data) override;

		virtual usize write(const void* data, usize bytes) override;
		virtual void flush() override;

	private:
		File(FILE* f);
		void swap(File& other);

		FILE* _file;
};

Y_ASSERT_SANE(File);

}
}

#endif // Y_IO_FILE_H
