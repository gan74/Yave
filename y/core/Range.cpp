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

#include "Range.h"
#include "Vector.h"
#include "String.h"
#include <y/test/test.h>
#include <iostream>

namespace y {
namespace core {

static_assert(std::is_same<typename decltype(range(vector(1, 2, 3)))::Element, int>::value, "Range::Element deduction error");
static_assert(!is_iterable<int>::value, "is_iterable<int> should be false");
static_assert(is_iterable<Vector<int>>::value, "is_iterable<Vector<int>> should be true");

y_test_func("Range hashing") {
	auto vec = range(0, 10).collect<Vector>();
	y_test_assert(vec.size() == 10);
	y_test_assert(hash(range(0, 10)) == hash(vec));
}

}
}
