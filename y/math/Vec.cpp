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

#include "Vec.h"
#include <y/test/test.h>
#include <iostream>

namespace y {
namespace math {

static_assert(std::is_trivially_copyable<Vec4>::value, "Vec<T> should be trivially copyable");

y_test_func("Vec creation") {
	Vec<2> a(1, 2);
	Vec<3> b(a, 3);
	Vec<3> c(0, a);
	Vec<6> d(a, 3, b);

	y_test_assert(d == Vec<6>(1, 2, 3, 1, 2, 3));

	Vec<3, i32> e(7);
	y_test_assert(e.x() == 7 && e.y() == 7 && e.z() == 7);
}

y_test_func("Vec zero") {
	Vec<6> v;
	y_test_assert(v.is_zero());

	v.w() = 1;
	y_test_assert(!v.is_zero());
	y_test_assert(v.length() == 1);
	y_test_assert(v.length2() == 1);

	auto v2 = v.normalized();
	y_test_assert((v2 - v).length2() < 0.0001);
}

y_test_func("Vec cross") {
	Vec<3> a(1, 0, 0);
	Vec<3> b(0, 1, 0);

	y_test_assert(a.cross(b) == Vec<3>(0, 0, 1));
}

y_test_func("Vec vec(...)") {
	auto v = vec(1, vec(2, 3), 4, vec(5));
	y_test_assert(v.x() == 1 && v.y() == 2 && v.z() == 3 && v.w() == 4 && v[4] == 5);
}

y_test_func("Vec operators") {
	y_test_assert(vec(1, 2, 3) + 1 == vec(2, 3, 4));
	y_test_assert(vec(1, 2, 3) * 2 == vec(2, 4, 6));
	y_test_assert(vec(1, 2, 3) - 1 == vec(0, 1, 2));
	y_test_assert(vec(2, 4, 6) / 2 == vec(1, 2, 3));

	y_test_assert(vec(1, 2, 3) / 2 == vec(0, 1, 1));
	y_test_assert(vec(1, 2, 3) * 0.5 == vec(0.5, 1, 1.5));

	static_assert(std::is_same<decltype(vec(1, 2, 3) / 2), Vec<3, int>>::value, "Invalid Vec operator coercion");
	static_assert(std::is_same<decltype(vec(1, 2, 3) * 0.5), Vec<3, double>>::value, "Invalid Vec operator coercion");
	static_assert(std::is_same<decltype(vec(1, 2.0, 3) * 2), Vec<3, double>>::value, "Invalid Vec operator coercion");
}

}
}
