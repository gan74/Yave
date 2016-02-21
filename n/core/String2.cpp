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
#include "String2.h"

namespace n {
namespace core {

char *String2::allocLong(uint len) {
	char *a = new char[len + 1];
	a[len] = 0;
	return a;
}

String2::String2() : s(ShortData()) {
}

String2::String2(const char *str, uint len) {
	if(len <= MaxShortSize) {
		s = ShortData(str, len);
	} else {
		l = LongData(str, len);
	}
}

uint String2::size() const {
	return isLong() ? l.length : s.length;
}

bool String2::isLong() const {
	return l.length.isLong;
}

char *String2::data() {
	return isLong() ? l.data : s.data;
}

const char *String2::data() const {
	return isLong() ? l.data : s.data;
}

}
}
