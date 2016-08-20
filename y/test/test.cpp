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

#include "test.h"
#include <y/utils.h>
#include <iostream>

namespace y {
namespace test {

const char *test_box_msg(const char *msg) {
	return msg;
}

void test_assert(const char *msg, bool sucess, const char *file, int line) {
	if(msg) {
		std::cout << msg << ": \t";
	}
	if(sucess) {
		std::cout << "ok" << std::endl;
	} else {
		fatal("failed!", file, line);
	}
}



}
}
