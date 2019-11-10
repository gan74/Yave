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

#include <y/test/test.h>
#include <y/serde3/serde.h>
#include <y/serde3/archives.h>
#include <y/serde3/poly.h>

#include <y/utils/name.h>
#include <y/utils/hash.h>

#include <y/core/Chrono.h>
#include <y/core/String.h>
#include <y/core/FixedArray.h>
#include <y/io2/File.h>
#include <y/io2/Buffer.h>

using namespace y;
using namespace serde3;

y_test_func("Test test") {
	y_test_assert(true);
}

struct Base {
	virtual ~Base() = default;

	virtual void print() {
		log_msg("Base");
	}

	y_serde3_poly_base(Base)
};

struct Derived : Base {
	int x = 16;

	void print() override {
		log_msg("Derived");
	}

	y_serde3(x)
	y_serde3_poly(Derived)
};

template<typename T>
struct Template : Base {
	T t = {};

	void print() override {
		log_msg(fmt("Template<%>", ct_type_name<T>()));
	}

	y_serde3(t)
	y_serde3_poly(Template)

};

int foobar() {
	Template<int> t;
	return t.t;
}

static_assert(has_serde3_poly_v<Derived*>);
static_assert(has_serde3_poly_v<Base*>);
static_assert(!has_serde3_poly_v<Base>);
static_assert(has_serde3_poly_v<Template<float>*>);




struct NestedStruct {
	float i = 3.14159f;

	y_serde3(i)
};

struct TestStruct {
	int x = 9;
	float y = 443;
	NestedStruct z;

	y_serde3(z, x, y)
};


auto poly_objects() {
	/*core::Vector<std::unique_ptr<Base>> v;
	v << std::make_unique<Derived>();
	v << std::make_unique<Template<int>>();
	v << std::make_unique<Derived>();
	v << nullptr;
	v << std::make_unique<Template<float>>();*/
	core::FixedArray<std::unique_ptr<Base>> v(5);
	v[0] = std::make_unique<Derived>();
	v[1] = std::make_unique<Template<int>>();
	v[2] = std::make_unique<Derived>();
	v[3] = nullptr;
	v[4] = std::make_unique<Template<float>>();
	return v;
}

auto simple_objects() {
	core::Vector<int> v;
	v << 4;
	v << 10;
	v << -9;
	v << 9999999;
	v << 0;
	return v;
}

int main() {{
		WritableArchive arc(std::move(io2::File::create("poly.txt").unwrap()));
		arc.serialize(poly_objects()).unwrap();
	}
	{
		ReadableArchive arc(std::move(io2::File::open("poly.txt").unwrap()));

		decltype(poly_objects()) col;
		arc.deserialize(col).unwrap();
		for(const auto& b : col) {
			if(b) {
				b->print();
			} else {
				log_msg("(null)");
			}
		}
	}


	/*usize count = 1000;
	{
		WritableArchive arc(std::move(io2::File::create("test.txt").unwrap()));
		TestStruct t{4, 5, {2.71727f}};
		for(usize i = 0; i != count; ++i) {
			arc.serialize(t).unwrap();
		}
	}
	{
		ReadableArchive arc(std::move(io2::File::open("test.txt").unwrap()));
		TestStruct t;

		Success s = Success::Full;
		{
			core::DebugTimer _("deserialize");
			for(usize i = 0; i != count; ++i) {
				s = arc.deserialize(t).unwrap();
			}
		}


		log_msg(fmt("status = %", s == Success::Full ? "full" : "partial"), s == Success::Full ? Log::Info : Log::Error);
		log_msg(fmt("{%, %, {%}}", t.x, t.y, t.z.i));
	}*/
	return 0;
}



