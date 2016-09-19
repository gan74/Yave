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

#include "AssocVector.h"
#include "String.h"
#include <y/test/test.h>

namespace y {
namespace core {

static_assert(std::is_same<usize, std::remove_reference<decltype(make_one<AssocVector<usize, usize>>()[usize(1)])>::type>::value, "AssocVector::operator[] returns the wrong type");

struct NonCopyableValue : NonCopyable {
	NonCopyableValue() : value(-1) {
	}

	NonCopyableValue(int v) : value(v) {
	}

	NonCopyableValue(NonCopyableValue&& other) : value(other.value) {
		other.value = -2;
	}

	NonCopyableValue &operator=(NonCopyableValue&& other) {
		value = std::exchange(other.value, -2);
		return *this;
	}

	int value;
};

y_test_func("AssocVector creation") {
	AssocVector<usize, usize> av;
	for(usize i = 0; i != 16; i++) {
		y_test_assert(av[i] == 0);
		av[i] = i + 1;
	}
	for(auto i : av) {
		y_test_assert(i.first == i.second - 1);
	}
}

y_test_func("AssocVector movable values") {
	AssocVector<usize, NonCopyableValue> av;
	for(usize i = 0; i != 16; i++) {
		av[i] = NonCopyableValue(i);
	}
	for(usize i = 0; i != av.size(); i++) {
		const auto& value = av[i];
		y_test_assert(value.value == int(i));
	}
}

y_test_func("AssocVector find") {
	AssocVector<usize, NonCopyableValue> av;
	for(usize i = 0; i != 16; i++) {
		av[i] = NonCopyableValue(i);
	}
	y_test_assert(range(av).find([](const auto& i) { return i.first == 7; })->second.value == 7);
	y_test_assert(range(av).find([](const auto& i) { return i.first < 0; }) == av.end());
	y_test_assert(range(av).find(-1) == av.end());
	y_test_assert(range(av).find(13)->second.value == 13);
}

}
}
