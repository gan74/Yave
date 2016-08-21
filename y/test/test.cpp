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
namespace detail {

const char *test_box_msg(const char *msg) {
	return msg;
}

void test_assert(const char *msg, void (*func)(TestResult *)) {
	if(msg) {
		std::cout << msg << ": \t";
	}
	TestResult res{true, nullptr, 0};
	func(&res);
	if(res.result) {
		std::cout << "ok" << std::endl;
	} else {
		fatal("failed!", res.file, res.line);
	}
}


}
}
}
