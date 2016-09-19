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
#ifndef YAVE_SPIRVDATA_H
#define YAVE_SPIRVDATA_H

#include <yave/yave.h>
#include <y/io/Ref.h>

namespace yave {

class SpirVData {

	public:
		SpirVData() = default;

		static SpirVData from_file(io::ReaderRef reader);

		usize size() const;
		const u32* data() const;

	private:
		SpirVData(core::Vector<u8>&& data);

		core::Vector<u8> _data;
};

}

#endif // YAVE_SPIRVDATA_H
