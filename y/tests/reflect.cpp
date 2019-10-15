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
#include <y/math/Vec.h>
#include <y/reflect/reflect.h>

#include <y/test/test.h>

namespace {
using namespace y;
using namespace y::core;

struct Trivial {
	int x = 1;
	float y = 2;
	math::Vec3 z = {3.0f, 4.0f, 5.0f};

	y_reflect(x, y, z)
};

struct Easy {
	Trivial t;
	std::tuple<core::String, int> str = {"a string", 7};

	y_reflect(str, t, -17)
};

struct Complex {
	core::Vector<Easy> a;
	core::Vector<int> b;
	int c;
	core::String d;

	y_reflect(a, b, d, c)
};


y_test_func("reflect trivial") {
	{
		usize iters = 0;
		Easy tr;
		tr.reflect([&](std::string_view, auto&&) { ++iters; });
		y_test_assert(iters == 11 + std::get<0>(tr.str).size());
	}

	{
		usize iters = 0;
		reflect::reflect([&](std::string_view, auto&&) { ++iters; }, std::pair<int, float>(1, 2.0f));
		y_test_assert(iters == 3);
	}
}

}
