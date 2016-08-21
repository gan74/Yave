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
#include "Vector.h"
#include <y/test/test.h>

namespace y {
namespace core {

y_test_func("Vector creation") {
	Vector<int> vec;
	y_test_assert(vec.size() == 0);
	y_test_assert(vec.capacity() == 0);
}

y_test_func("Vector append") {
	Vector<int> vec;

	vec.set_capacity(19);
	y_test_assert(vec.capacity() >= 19);

	vec.append(0);
	vec.append(1);
	y_test_assert(vec.size() == 2);

	for(int i = 0; i != 18; i++) {
		vec.append(i);
	}

	y_test_assert(vec.size() == 20);
	y_test_assert(vec.capacity() >= 20);
}

y_test_func("Vector shrink") {
	const usize max = 256;

	Vector<int> vec;

	while(vec.size() != max) {
		vec.append(0);
	}

	y_test_assert(vec.size() == max);
	y_test_assert(vec.capacity() >= max);

	vec.set_capacity(max / 2);
	y_test_assert(vec.size() == vec.capacity());
	y_test_assert(vec.capacity() == max / 2);
}

y_test_func("Vector clear") {
	const usize max = 256;

	Vector<int> vec;

	while(vec.size() != max) {
		vec.append(0);
	}
	vec.clear();
	y_test_assert(!vec.size());
	y_test_assert(!vec.capacity());

	while(vec.size() != max * 2) {
		vec.append(0);
	}
	vec.clear();
	y_test_assert(!vec.size());
	y_test_assert(!vec.capacity());
}


}
}
