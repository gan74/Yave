/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

#include <y/core/ArrayView.h>
#include <y/core/SmallVector.h>
#include <y/test/test.h>

#include <y/core/String.h>

#include <vector>
#include <numeric>

using namespace y;
using namespace y::core;

static usize test_func_c(ArrayView<char> a) {
	return a.size();
}

static usize test_func(ArrayView<int> a) {
	return a.size();
}

static usize test_func_nc(ArrayView<NonCopyable> a) {
	return a.size();
}


y_test_func("ArrayView creation") {
	y_test_assert(test_func({1, 2, 3}) == 3);

	auto vec = Vector({1, 2, 3, 4});
	y_test_assert(test_func(vec) == 4);

	y_test_assert(test_func(SmallVector<int>({1, 2, 3, 4})) == 4);

	std::vector<int> std_vec(12);
	std::iota(std_vec.begin(), std_vec.end(), 17);
	y_test_assert(test_func(std_vec) == 12);

	y_test_assert(test_func_c("12345") == 6);
	y_test_assert(test_func_c("12345"_s) == 5);

	const int arr[] = {1, 7, 9};
	y_test_assert(test_func(arr) == 3);

	y_test_assert(test_func(nullptr) == 0);
	y_test_assert(test_func(17) == 1);
}

y_test_func("ArrayView of non-copyables") {
	y_test_assert(test_func_nc(NonCopyable()) == 1);
	y_test_assert(test_func_nc({NonCopyable(), NonCopyable()}) == 2);

	/*NonCopyable nc;
	y_test_assert(test_func_nc({nc, NonCopyable()}) == 2);*/
}
