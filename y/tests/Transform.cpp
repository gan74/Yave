/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#include <y/math/Transform.h>
#include <y/test/test.h>

namespace {
using namespace y;
using namespace y::math;


y_test_func("Transform set basis") {
	Transform<> tr;
	tr.set_basis(Vec3(1.0f, 0.0f, 0.0f), Vec3(0.0f, 0.0f, 1.0f));

	y_test_assert(tr.up() == Vec(0.0f, 0.0f, 1.0f));
}

y_test_func("Transform decompose basic") {
	auto quat = Quaternion<>::from_euler(to_rad(90.0f), 0.0f, to_rad(90.0f));
	Vec3 scale(5.0f);
	Vec3 pos(1.0f, 7.0, 9.0f);
	Transform<> tr(pos, quat, scale);

	auto [p, q, s] = tr.decompose();
	y_test_assert(p == pos);
	y_test_assert(q == quat);
	y_test_assert(s == scale);
}

y_test_func("Transform decompose") {
	int step = 7;
	for(int ph = -180; ph < 180; ph += step) {
		for(int y = -180; y < 180; y += step) {
			for(int r = -180; r < 180; r += step) {
				auto quat = Quaternion<>::from_euler(to_rad(ph), to_rad(y), to_rad(r));
				Vec3 pos(y, r, ph);
				Vec3 scale(1.0f + pos.length() * 0.1f);
				Transform<> tr(pos, quat, scale);

				auto [p, q, s] = tr.decompose();

				Vec3 v(0.5f, 0.75f, 1.0f);
				y_test_assert((p - pos).length() < 0.001f);
				y_test_assert((q(v) - quat(v)).length() < 0.001f);
				y_test_assert((s - scale).length() < 0.001f);
			}
		}
	}
}
}
