/*******************************
Copyright (C) 2013-2014 grégoire ANGERAND

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

#include "OutputStream.h"


namespace n {
namespace io {

DataOutputStream::DataOutputStream(OutputStream *s) : stream(s) {
}

bool DataOutputStream::canWrite() const {
	return stream->canWrite();
}

void DataOutputStream::flush() {
	stream->flush();
}

uint DataOutputStream::writeBytes(const char *b, uint len) {
	return stream->writeBytes(b, len);
}

} // io
} // n
