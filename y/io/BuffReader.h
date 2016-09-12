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
#ifndef Y_IO_BUFFREADER_H
#define Y_IO_BUFFREADER_H

#include "Ref.h"

namespace y {
namespace io {

class BuffReader : public Reader {

	public:
		BuffReader(ReaderRef&& r, usize buff_size = 512);
		BuffReader(const ReaderRef& r, usize buff_size = 512);

		virtual ~BuffReader();

		BuffReader(BuffReader&& other);
		BuffReader& operator=(BuffReader&& other);

		virtual bool at_end() const override;

		virtual usize read(void *data, usize bytes) override;
		virtual usize read_all(core::Vector<u8>& data) override;

	private:
		BuffReader(usize buff_size = 0);
		void swap(BuffReader& other);

		usize _size;
		usize _offset;
		usize _used;
		u8* _buffer;

		ReaderRef _inner;
};

Y_ASSERT_SANE(BuffReader);

}
}
#endif // Y_IO_BUFFREADER_H
