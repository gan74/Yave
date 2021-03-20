/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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
#include <y/math/Quaternion.h>
#include <y/math/math.h>
#include <y/core/String.h>
#include <y/test/test.h>

namespace {
using namespace y;
using namespace y::math;

y_test_func("Quaternion from_euler pitch") {
    // Rotation around X
    const auto q = Quaternion<>::from_euler(to_rad(90), 0, 0);

    y_test_assert(q({0.0f, 1.0f, 0.0f}).z() > 0.99f);
    y_test_assert(q({0.0f, 0.0f, 1.0f}).y() < -0.99f);
    y_test_assert(to_deg(q.pitch()) == 90.0f);
}

y_test_func("Quaternion from_euler yaw") {
    // Rotation around Z
    const auto q = Quaternion<>::from_euler(0, to_rad(90), 0);

    y_test_assert(q({1.0f, 0.0f, 0.0f}).y() > 0.99f);
    y_test_assert(q({0.0f, 0.0f, 1.0f}).z() > 0.99f);
    y_test_assert(to_deg(q.yaw()) == 90.0f);
}

y_test_func("Quaternion from_euler roll") {
    // Rotation around Y
    const auto q = Quaternion<>::from_euler(0, 0, to_rad(90));

    y_test_assert(q({1.0f, 0.0f, 0.0f}).z() < -0.99f);
    y_test_assert(q({0.0f, 0.0f, 1.0f}).x() > 0.99f);
    y_test_assert(to_deg(q.roll()) == 90.0f);
}

y_test_func("Quaternion default values") {
    const Quaternion<> quat;

    y_test_assert(quat({1.0f, 0.0f, 0.0f}).x() > 0.99f);
    y_test_assert(quat({0.0f, 1.0f, 0.0f}).y() > 0.99f);
    y_test_assert(quat({0.0f, 0.0f, 1.0f}).z() > 0.99f);
}

y_test_func("Quaternion rotation") {
    auto axis = Vec3{1.0f, 0.3f, 0.2f}.normalized();
    const float angle = 0.8f;

    const auto mat = rotation(axis, angle);
    const auto quat = Quaternion<>::from_axis_angle(axis, angle);

    const Vec3 t1{1.0f, -5.0f, 1.62f};
    y_test_assert(((mat * Vec4(t1, 0.0f)).to<3>() - quat(t1)).length2() < 0.01f);
}

y_test_func("Quaternion euler") {
    const int step = 7;
    for(int p = -180; p < 180; p += step) {
        for(int y = -180; y < 180; y += step) {
            for(int r = -180; r < 180; r += step) {
                const auto quat = Quaternion<>::from_euler(to_rad(p), to_rad(y), to_rad(r));
                const auto q2 = Quaternion<>::from_euler(quat.pitch(), quat.yaw(), quat.roll());;

                const Vec3 v(0.5f, 0.75f, 1.0f);
                y_test_assert((q2(v) - quat(v)).length() < 0.001f);
            }
        }
    }
}
}

