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

#include "Functor.h"
#include <y/test/test.h>

namespace y {
namespace core {

auto create_func() {
	return []() {
		return 4;
	};
}

y_test_func("Function creation") {
	int i = 0;
	auto inc = function([&i]() { i++; });

	y_test_assert(!i);
	inc();
	y_test_assert(i == 1);

	{
		inc = function([&i]() { i--; });
	}

	inc();
	y_test_assert(!i);

	{
		auto dec = std::move(inc);
		dec();
		y_test_assert(i == -1);
	}

	inc = create_func();
	inc();
	y_test_assert(i == -1);
}

y_test_func("Function void boxing") {
	int i = 0;
	auto inc = function([&i]() { return i++; });
	inc();
	y_test_assert(i == 1);
}

y_test_func("Functor creation") {
	int i = 0;
	auto inc = functor([&i]() { i++; });

	inc();
	y_test_assert(i == 1);

	auto dec = inc;

	dec();
	y_test_assert(i == 2);
	{
		auto d = functor([&i]() { i--; });
		dec = d;
	}
	dec();
	dec();
	y_test_assert(!i);

	dec = create_func();
	dec();
	y_test_assert(!i);

}


}
}
