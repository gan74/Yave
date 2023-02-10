/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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
#include <y/utils/traits.h>

namespace {
using namespace y;
using namespace y::math;

static_assert(is_iterable_v<Vec3>);

y_test_func("Vec creation") {
    const Vec<2> a(1, 2);
    const Vec<3> b(a, 3);
    const Vec<3> c(0, a);
    const Vec<6> d(a, 3, b);

    y_test_assert(c == Vec<3>(0, 1, 2));
    y_test_assert(d == Vec<6>(1, 2, 3, 1, 2, 3));

    const Vec<3, i32> e(7);
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
    const Vec<3> a(1, 0, 0);
    const Vec<3> b(0, 1, 0);

    y_test_assert(a.cross(b) == Vec<3>(0, 0, 1));
}

y_test_func("Vec Vec(...)") {
    const auto v = Vec<5, i32>(1, Vec2i(2, 3), 4, Vec<1, i32>(5));
    y_test_assert(v.x() == 1 && v.y() == 2 && v.z() == 3 && v.w() == 4 && v[4] == 5);
}

y_test_func("Vec operators") {
    y_test_assert(Vec3i(1, 2, 3) + 1 == Vec3i(2, 3, 4));
    y_test_assert(Vec3i(1, 2, 3) * 2 == Vec3i(2, 4, 6));
    y_test_assert(Vec3i(1, 2, 3) - 1 == Vec3i(0, 1, 2));
    y_test_assert(Vec3i(2, 4, 6) / 2 == Vec3i(1, 2, 3));

    y_test_assert(Vec3(1, 2, 3) / 2.0f == Vec3(0.5f, 1.0f, 1.5f));
    y_test_assert(Vec3d(1, 2, 3) * 0.5 == Vec3d(0.5, 1, 1.5));

    static_assert(std::is_same_v<decltype(Vec3i(1, 2, 3) / 2), Vec<3, int>>, "Invalid Vec operator coercion");
    static_assert(std::is_same_v<decltype(Vec3(1, 2, 3) * 0.5), Vec<3, double>>, "Invalid Vec operator coercion");
    static_assert(std::is_same_v<decltype(Vec3d(1, 2.0, 3) * 2), Vec<3, double>>, "Invalid Vec operator coercion");
}
}

