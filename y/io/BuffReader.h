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

#include "Read.h"

namespace y {
namespace io {

class BuffReader : public Read {
	public:
		BuffReader(Read &r);
		virtual ~BuffReader();

		BuffReader(BuffReader &&other);
		BuffReader &operator=(BuffReader &&other);

		virtual usize read(void *data, usize bytes) override;
		virtual usize read_all(core::Vector<u8> &data) override;

	private:
		BuffReader();
		void swap(BuffReader &other);

		usize buffer_size;
		usize buffer_offset;
		usize buffer_used;
		u8 *buffer;

		Read *inner;
};

}
}
#endif // Y_IO_BUFFREADER_H
