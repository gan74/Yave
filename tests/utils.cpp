/*******************************
Copyright (c) 2016-2017 Gr�goire Angerand

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

#include <y/utils.h>
#include <y/test/test.h>

#include <y/core/Vector.h>

using namespace y;

struct DRaii : NonCopyable {
	DRaii(usize& i) : _i(&i) {
		++(*_i);
	}

	~DRaii() {
		if(_i) {
			--(*_i);
		}
	}

	DRaii(DRaii&& r) : _i(std::exchange(r._i, nullptr)) {
	}

	usize* _i = nullptr;
};

y_test_func("utils do_not_destroy") {
	usize i = 0;
	{
		DRaii r(i);
		y_test_assert(i == 1);
		do_not_destroy(std::move(r));
	}
	y_test_assert(i == 1);
}


y_test_func("utils hash") {
	y_test_assert(hash(1, 2, 3) != hash(1, 2, 5));
	y_test_assert(hash<std::initializer_list<int>>({1, 2, 3}) == hash(core::Vector({1, 2, 3})));
}


