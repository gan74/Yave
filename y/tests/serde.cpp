/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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
#include <y/core/Vector.h>
#include <y/core/String.h>
#include <y/io2/Buffer.h>
#include <y/math/math.h>
#include <y/serde2/archives.h>

#include <y/io/Ref.h>

#include <y/test/test.h>

namespace {
using namespace y;
using namespace y::core;
using namespace y::serde2;

struct Trivial {
	int x;
	float y;
	math::Vec3 z;

	bool operator==(const Trivial& other) const {
		return std::tie(x, y, z) == std::tie(other.x, other.y, other.z);
	}
};

struct Easy {
	Trivial t;
	std::tuple<core::String, int> str;

	y_serde2(str, t, check(-17))

	bool operator==(const Easy& other) const {
		return std::tie(t, str) == std::tie(other.t, other.str);
	}
};

struct Complex {
	core::Vector<Easy> a;
	core::Vector<int> b;
	int c;
	core::String d;

	y_serde2(a, b, d, c)

	bool operator==(const Complex& other) const {
		return std::tie(a, b, c, d) == std::tie(other.a, other.b, other.c, other.d);
	}
};

y_test_func("serde trivial") {
	io2::Buffer buffer;
	Trivial tri{7, 3.1416, {0.0f, 1.0f, 2.7f}};

	{
		io2::Writer writer(buffer);
		WritableArchive<> ar(writer);
		unused(ar(tri));
	}
	{
		io2::Reader reader(buffer);
		serde2::ReadableArchive<> ar(reader);
		Trivial t;
		unused(ar(t));
		y_test_assert(t == tri);
	}
}

y_test_func("serde easy") {
	io2::Buffer buffer;
	Trivial tri{7, 3.1416f, {0.0f, 1.0f, 2.7f}};
	Easy es{tri, {"some long long long, very long, even longer string (probably to bypass SSO)", 99999}};

	{
		io2::Writer writer(buffer);
		WritableArchive<> ar(writer);
		unused(ar(es));
	}
	{
		io2::Reader reader(buffer);
		serde2::ReadableArchive<> ar(reader);
		Easy e;
		unused(ar(e));
		y_test_assert(e == es);
	}
}

y_test_func("serde complex") {
	io2::Buffer buffer;
	Trivial t0{7, 3.1416f, {0.0f, 1.0f, 2.7f}};
	Trivial t1{641, -4.6f, {2.1828, 7.9f, -9999.0f}};
	Trivial t2{-9256, t0.z.dot(t1.z), t0.z.cross(t1.z)};
	Easy e0{t1, {"some long long long, very long, even longer string (probably to bypass SSO)", 99999}};
	Easy e1{t2, {"flublbu", __LINE__}};
	Easy e2{t0, {__FUNCTION__, -8941}};

	Complex comp{{e0, e1, e2}, {1, 2, 3, 4, 5, 6, 7, 999}, -798, "some other string"};

	{
		io2::Writer writer(buffer);
		WritableArchive<> ar(writer);
		unused(ar(e2, comp, t1));
	}
	{
		io2::Reader reader(buffer);
		serde2::ReadableArchive<> ar(reader);

		Easy e;
		Complex c;
		Trivial t;

		unused(ar(e));
		unused(ar(c, t));

		y_test_assert(e == e2);
		y_test_assert(c == comp);
		y_test_assert(t == t1);
	}
}

}
