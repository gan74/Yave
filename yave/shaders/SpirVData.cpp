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
#include "SpirVData.h"

namespace yave {

SpirVData SpirVData::from_file(io::ReaderRef reader) {
	Y_TODO(optimise)
	core::Vector<u8> content = reader->read_all();
	core::Vector<u32> spriv32(content.size() / 4, 0);
	memcpy(spriv32.begin(), content.begin(), content.size());
	return SpirVData(spriv32);
}



SpirVData::SpirVData(const core::Vector<u32>& data) : _data(data) {
}

usize SpirVData::size() const {
	return _data.size() * 4;
}

const u32* SpirVData::data() const {
	return _data.begin();
}

bool SpirVData::is_empty() const {
	return _data.is_empty();
}

}
