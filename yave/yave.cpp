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

#include "yave.h"
#include <fstream>

namespace yave {

core::Vector<u8> read_file(const core::String& file_name) {
	std::ifstream file(file_name, std::ios::ate | std::ios::binary);
	if(!file.is_open()) {
		fatal("Unable to open file");
	}
	usize len = file.tellg();
	core::Vector<u8> buffer(len, 0);

	file.seekg(0);
	file.read(reinterpret_cast<char *>(buffer.begin()), len);
	file.close();

	return buffer;
}

}
