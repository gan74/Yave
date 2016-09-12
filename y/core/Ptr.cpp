/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/
#include "Ptr.h"
#include <y/test/test.h>

namespace y {
namespace core {

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


y_test_func("Ptr RAII") {
	bool exists = false;

	{
		auto p = ptr(Raii(exists));
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



}
}
