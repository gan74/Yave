/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

using namespace y;
using namespace y::math;

y_test_func("Quaternion from_euler yaw") {
	auto z90 = Quaternion<>::from_euler(to_rad(90), 0, 0);

	y_test_assert(z90({1.0f, 0.0f, 0.0f}).y() > 0.99f);
	y_test_assert(z90({0.0f, 0.0f, 1.0f}).z() > 0.99f);
}

y_test_func("Quaternion default") {
	Quaternion<> quat;

	y_test_assert(quat({1.0f, 0.0f, 0.0f}).x() > 0.99f);
	y_test_assert(quat({0.0f, 1.0f, 0.0f}).y() > 0.99f);
	y_test_assert(quat({0.0f, 0.0f, 1.0f}).z() > 0.99f);
}


y_test_func("Quaternion rotation") {
	auto axis = Vec3{1.0f, 0.3f, 0.2f}.normalized();
	float angle = 0.8f;

	auto mat = rotation(axis, angle);
	auto quat = Quaternion<>::from_axis_angle(axis, angle);

	Vec3 t1{1.0f, -5.0f, 1.62f};
	y_test_assert(((mat * Vec4(t1, 0.0f)).to<3>() - quat(t1)).length2() < 0.01f);
}
