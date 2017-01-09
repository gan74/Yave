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

#include <y/core/Range.h>
#include <y/core/Vector.h>
#include <y/core/String.h>
#include <y/test/test.h>

using namespace y;
using namespace y::core;

static_assert(std::is_same<typename decltype(range(vector(1, 2, 3)))::Element, int>::value, "Range::Element deduction error");
static_assert(!is_iterable<int>::value, "is_iterable<int> should be false");
static_assert(is_iterable<Vector<int>>::value, "is_iterable<Vector<int>> should be true");

y_test_func("Range hashing") {
	auto vec = range(0, 10).collect<Vector>();
	y_test_assert(vec.size() == 10);
	y_test_assert(hash(range(0, 10)) == hash(vec));
}
