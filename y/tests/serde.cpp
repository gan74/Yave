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
#include <y/serde2/serde.h>

#include <y/test/test.h>

namespace {
using namespace y;
using namespace y::core;
using namespace y::serde2;

struct Func {
	y_serialize2(v, (v.size() ? u32(7) : u32(9)))
	y_deserialize2(v, func([this](u32 x) { s = x + 1; }))

	core::Vector<int> v;
	u32 s = 0;
};

struct DummyWriter : public io2::Writer {
	DummyWriter() = default;

	io2::WriteResult write(const u8*, usize) override {
		return core::Ok();
	}

	io2::FlushResult flush() override {
		return core::Ok();
	}
};


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

struct RaiiGuard : DummyWriter {
	bool* alive = nullptr;

	RaiiGuard(bool *a) : alive(a) {
	}

	RaiiGuard(RaiiGuard&& r) {
		std::swap(alive, r.alive);
	}

	RaiiGuard& operator=(RaiiGuard&& r) {
		std::swap(alive, r.alive);
		return *this;
	}

	~RaiiGuard() {
		if(alive) {
			*alive = false;
		}
	}
};

y_test_func("serde trivial") {
	io2::Buffer buffer;
	Trivial tri{7, 3.1416, {0.0f, 1.0f, 2.7f}};

	{
		WritableArchive ar(buffer);
		ar(tri).unwrap();
	}
	{
		serde2::ReadableArchive ar(buffer);
		Trivial t;
		ar(t).unwrap();
		y_test_assert(t == tri);
	}
}

y_test_func("serde easy") {
	io2::Buffer buffer;
	Trivial tri{7, 3.1416f, {0.0f, 1.0f, 2.7f}};
	Easy es{tri, {"some long long long, very long, even longer string (probably to bypass SSO)", 99999}};

	{
		WritableArchive ar(buffer);
		ar(es).unwrap();
	}
	{
		serde2::ReadableArchive ar(buffer);
		Easy e;
		ar(e).unwrap();
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
		WritableArchive ar(buffer);
		ar(e2, comp, t1).unwrap();
	}
	{
		serde2::ReadableArchive ar(buffer);

		Easy e;
		Complex c;
		Trivial t;

		ar(e).unwrap();
		ar(c, t).unwrap();

		y_test_assert(e == e2);
		y_test_assert(c == comp);
		y_test_assert(t == t1);
	}
}

y_test_func("serde RAII") {
	Trivial tri{7, 3.1416f, {0.0f, 1.0f, 2.7f}};
	Easy es{tri, {"some long long long, very long, even longer string (probably to bypass SSO)", 99999}};

	{
		bool alive = true;
		{
			RaiiGuard r(&alive);
			WritableArchive ar(r);
			y_test_assert(alive);
			ar(es).unwrap();
			y_test_assert(alive);
		}
		y_test_assert(!alive);
	}

	{
		bool alive = true;
		{
			RaiiGuard r(&alive);
			auto ar = WritableArchive(r);
			y_test_assert(alive);
			ar(es).unwrap();
			y_test_assert(alive);
		}
		y_test_assert(!alive);
	}

	{
		bool alive = true;
		y_test_assert(alive);
		RaiiGuard r(&alive);
		WritableArchive a(r);
		a(es, func([&] { y_test_assert(alive); })).unwrap();
		y_test_assert(!alive);
	}
}


y_test_func("serde func") {
	io2::Buffer buffer;
	{
		Func f;
		f.v = {1, 2, 3};
		WritableArchive ar(buffer);
		f.serialize(ar).unwrap();
	}
	{
		Func f;
		ReadableArchive ar(buffer);
		f.deserialize(ar).unwrap();
		y_test_assert(f.v == core::ArrayView<int>({1, 2, 3}));
		y_test_assert(f.s == 8);
	}
}

}
