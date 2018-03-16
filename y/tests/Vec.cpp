/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/

#include <y/math/Vec.h>
#include <y/test/test.h>

using namespace y;
using namespace y::math;

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

y_test_func("Vec Vec(...)") {
	auto v = Vec(1, Vec(2, 3), 4, Vec(5));
	y_test_assert(v.x() == 1 && v.y() == 2 && v.z() == 3 && v.w() == 4 && v[4] == 5);
}

y_test_func("Vec operators") {
	y_test_assert(Vec(1, 2, 3) + 1 == Vec(2, 3, 4));
	y_test_assert(Vec(1, 2, 3) * 2 == Vec(2, 4, 6));
	y_test_assert(Vec(1, 2, 3) - 1 == Vec(0, 1, 2));
	y_test_assert(Vec(2, 4, 6) / 2 == Vec(1, 2, 3));

	y_test_assert(Vec(1, 2, 3) / 2 == Vec(0, 1, 1));
	y_test_assert(Vec(1, 2, 3) * 0.5 == Vec(0.5, 1, 1.5));

	static_assert(std::is_same_v<decltype(Vec(1, 2, 3) / 2), Vec<3, int>>, "Invalid Vec operator coercion");
	static_assert(std::is_same_v<decltype(Vec(1, 2, 3) * 0.5), Vec<3, double>>, "Invalid Vec operator coercion");
	static_assert(std::is_same_v<decltype(Vec(1, 2.0, 3) * 2), Vec<3, double>>, "Invalid Vec operator coercion");
}
