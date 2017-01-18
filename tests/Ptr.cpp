/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#include <y/core/Ptr.h>
#include <y/test/test.h>

using namespace y;
using namespace y::core;

struct Raii : NonCopyable {
	static bool ded;

	Raii(bool& b) : e(&b) {
		*e = true;
	}

	Raii(Raii&& r) : e(r.e) {
		r.e = &ded;
	}

	~Raii() {
		*e = false;
	}

	bool* e;
};
bool Raii::ded = false;


y_test_func("Unique RAII") {
	bool exists = false;

	{
		auto p = unique(Raii(exists));
		y_test_assert(exists);
		{
			y_test_assert(exists);
		}
		y_test_assert(exists);
	}
	y_test_assert(!exists);
}


y_test_func("Rc RAII") {
	bool exists = false;
	{
		auto p = rc(Raii(exists));
		y_test_assert(exists);
		{
			y_test_assert(exists);
		}
		y_test_assert(exists);
	}
	y_test_assert(!exists);

}


y_test_func("Rc count") {
	bool exists = false;
	bool exists2 = false;
	{
		auto p = rc(Raii(exists));
		y_test_assert(p.ref_count() == 1);
		y_test_assert(exists);

		{
			auto p2 = p;
			y_test_assert(p.ref_count() == 2);
			y_test_assert(p2.ref_count() == 2);
			y_test_assert(exists);
		}
		y_test_assert(p.ref_count() == 1);
		y_test_assert(exists);

		{
			auto p2 = rc(Raii(exists2));
			y_test_assert(p.ref_count() == 1);
			y_test_assert(p2.ref_count() == 1);
			p = p2;
			y_test_assert(p.ref_count() == 2);
			y_test_assert(p2.ref_count() == 2);
			y_test_assert(!exists);
			y_test_assert(exists2);
		}
		y_test_assert(exists2);
		y_test_assert(p.ref_count() == 1);
	}
	y_test_assert(!exists2);
	y_test_assert(!exists);
}


