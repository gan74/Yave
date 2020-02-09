/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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
#include <y/core/SparseVector.h>
#include <y/test/test.h>
#include <vector>
#include <memory>

namespace {
using namespace y;
using namespace y::core;

y_test_func("SparseVector creation") {
	SparseVector<void, u32> vec;
	y_test_assert(vec.size() == 0);
	y_test_assert(!vec.has(0));
}

y_test_func("SparseVector insert even") {
	SparseVector<void, u32> vec;

	const u32 max = 1024;
	for(u32 i = 0; i != max; ++i) {
		vec.insert(i * 2);
	}
	y_test_assert(vec.size() == max);

	for(u32 i = 0; i != max; ++i) {
		y_test_assert(vec.has(i * 2));
		y_test_assert(!vec.has(i * 2 + 1));
	}
}

y_test_func("SparseVector erase even") {
	SparseVector<void, u32> vec;

	const u32 max = 1024;
	for(u32 i = 0; i != max; ++i) {
		vec.insert(i);
	}
	y_test_assert(vec.size() == max);

	usize erased = 0;
	for(u32 i = 0; i != max; ++i) {
		y_test_assert(vec.has(i));
		if(i % 2) {
			vec.erase(i);
			y_test_assert(!vec.has(i));
			++erased;
		}
	}

	y_test_assert(vec.size() + erased == max);

	for(u32 i = 0; i != max; ++i) {
		y_test_assert(vec.has(i) == (i % 2 == 0));
	}
}
}


