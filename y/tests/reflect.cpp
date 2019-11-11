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
#include <y/core/Vector.h>
#include <y/core/String.h>
#include <y/math/Vec.h>
#include <y/reflect/reflect.h>

#include <y/io2/Buffer.h>

#include <y/test/test.h>

namespace {
using namespace y;
using namespace y::core;

struct Trivial {
	int x = 1;
	float y = 2;
	math::Vec3 z = {3.0f, 4.0f, 5.0f};

	y_reflect(x, y, z)

	bool operator==(const Trivial& other) const {
		return std::tie(x, y, z) == std::tie(other.x, other.y, other.z);
	}
};

struct Easy {
	Trivial t;
	std::tuple<core::String, int> str = {"a string", 7};

	y_reflect(str, t)

	bool operator==(const Easy& other) const {
		return std::tie(t, str) == std::tie(other.t, other.str);
	}
};

struct Complex {
	core::Vector<Easy> a;
	core::Vector<int> b;
	int c = 9;
	core::String d;

	y_reflect(a, b, d, c)

	bool operator==(const Complex& other) const {
		return std::tie(a, b, c, d) == std::tie(other.a, other.b, other.c, other.d);
	}
};

struct Polymorphic {
	virtual ~Polymorphic() {
	}

	int x;

	y_reflect_polymorphic(x)
};

struct Derived : public Polymorphic {
	virtual ~Derived() {
	}

	float y;

	y_reflect_base(Polymorphic)
	y_reflect_polymorphic(y)
};

struct WithConst {
	const int a = 4;
	int b = 5;

	y_reflect(a, b)
};

y_test_func("reflect polymorphic") {
	reflect::RuntimeData data = reflect::reflection_data<Polymorphic>();
	y_test_assert(data.type.flags.is_polymorphic);
	y_test_assert(data.members.size() == 1);
	y_test_assert(data.members[0].name == "x");
}

y_test_func("reflect inheritance") {
	reflect::RuntimeData data = reflect::reflection_data<Derived>();
	y_test_assert(data.type.flags.is_polymorphic);
	y_test_assert(data.members.size() == 1);
	y_test_assert(data.members[0].name == "y");

	y_test_assert(data.parents.size() == 1);
	y_test_assert(data.parents[0].name == "Polymorphic");

	std::unique_ptr<Polymorphic> poly = std::make_unique<Derived>();
	y_test_assert(reflect::reflection_data(*poly).type == reflect::type<Derived>());
}

y_test_func("reflect pointer") {
	auto cx = std::make_unique<Complex>();
	reflect::RuntimeData data = reflect::reflection_data(cx.get());
	y_test_assert(data.type.flags.is_pointer);
	y_test_assert(data.members.size() == 4);
	y_test_assert(data.members[3].type.name == "int");
	y_test_assert(data.members[3].name == "c");
	y_test_assert(data.members[3].get<int>(cx.get()) == 9);
}

y_test_func("reflect set") {
	Trivial refl;
	Trivial* refl_ptr = &refl;

	y_test_assert(!reflect::reflection_data(refl).type.flags.is_pointer);
	y_test_assert(reflect::reflection_data(refl_ptr).type.flags.is_pointer);

	{
		auto data = reflect::reflection_data(refl_ptr);
		y_test_assert(data.members[1].name == "y");
		data.members[1].get<float>(refl_ptr) = 13.0f;
	}

	y_test_assert(refl.x == Trivial{}.x);
	y_test_assert(refl.z == Trivial{}.z);
	y_test_assert(refl.y == 13.0f);
}

static constexpr usize count_members(const reflect::RuntimeData& data) {
	usize total = 0;
	for(auto&& m : data.members) {
		total += 1 + count_members(m.type.reflection_data());
	}
	for(auto&& p : data.parents) {
		for(auto&& m : p.type.reflection_data().members) {
			total += 1 + count_members(m.type.reflection_data());
		}
	}
	return total;
}

y_test_func("reflect constexpr") {
	constexpr usize members = count_members(Trivial{}.reflection_data());
	y_test_assert(members == 3);

	Complex cx;
	constexpr usize cx_members = count_members(cx.reflection_data());
	y_test_assert(cx_members == 4);
}



[[maybe_unused]]
static void print_refl(const reflect::RuntimeData& data, usize indent = 0) {
	/*if(!data.type.flags.has_reflection) {
		return;
	}*/
	auto in = [=]() {
		core::String line;
		for(usize i = 0; i != indent; ++i) {
			line += "  ";
		}
		return line;
	};

	for(auto&& m : data.members) {
		auto l = in();
		fmt_into(l, "%{% %} % ", m.type.name, m.type.flags.is_trivially_copyable, m.type.flags.has_reflection, m.name);
		log_msg(l);
		print_refl(m.type.reflection_data(), indent + 1);
	}
}


}
