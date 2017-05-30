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


#include <y/math/math.h>


using namespace y;
using namespace y::math;

static Vec3 X(1.0f, 0.0f, 0.0f);
static Vec3 Y(0.0f, 1.0f, 0.0f);
static Vec3 Z(0.0f, 0.0f, 1.0f);

template<usize N>
static bool eq(const Vec<N>& a, const Vec<N>& b) {
	return (a - b).length2() < 0.001f;
}

template<usize N>
static bool not_n(const Vec<N>& a) {
	return a.length2() > 0.001f;
}

template<usize N>
static bool eq_not_n(const Vec<N>& a, const Vec<N>& b) {
	return not_n(a) && eq(a, b);
}


y_test_func("math basic rotation") {
	auto rot = rotation(pi<float>, Z);
	y_test_assert(eq_not_n(rot * Vec4(X, 0.0f), Vec4(-X, 0.0f)));
}



