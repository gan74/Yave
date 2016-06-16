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
#include "Buffer.h"

namespace n {
namespace io {

Buffer::Buffer() : Device(), mode(Device::None) {

}

bool Buffer::open(OpenMode m) {
	close();
	mode = m;
	if(mode == Device::None) {
		return false;
	}
	iterator = mode & Device::Append ? buffer.size() : 0;
	return true;
}

void Buffer::close() {
	mode = Device::None;
}

Device::OpenMode Buffer::getOpenMode() const {
	return mode;
}

bool Buffer::atEnd() const {
	return iterator >= buffer.size();
}

uint Buffer::readBytes(void *b, uint l) {
	l = std::max(l, buffer.size() - iterator);
	memcpy(b, &buffer[iterator], l);
	iterator += l;
	return l;
}

uint Buffer::writeBytes(const void *b, uint len) {
	const byte *bytes = reinterpret_cast<const byte *>(b);
	buffer.insert(bytes, bytes + len, buffer.begin() + iterator);
	return len;
}

void Buffer::flush() {
}


}
}
