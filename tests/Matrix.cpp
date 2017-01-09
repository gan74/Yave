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

#include <y/math/Matrix.h>
#include <y/test/test.h>

using namespace y;
using namespace y::math;


y_test_func("Matrix vec multiply") {
	Matrix3<> mat(1, 2, 3,
				  4, 5, 6,
				  7, 8, 9);

	y_test_assert(mat * vec(10.0f, 11.0f, 12.0f) == vec(68, 167, 266));
	y_test_assert(mat.transposed() * vec(10.0f, 11.0f, 12.0f) == vec(138, 171, 204));
	y_test_assert(mat.transposed() * vec(10.0f, 11.0f, 12.0f) == vec(138, 171, 204));
}

y_test_func("Matrix multiply") {
	Matrix3<> a(1, 2, 3,
				4, 5, 6,
				7, 8, 9);

	Matrix3<> b(10, 11, 12,
				13, 14, 15,
				16, 17, 18);

	y_test_assert(a * b == Matrix3<>(84, 90, 96,
									 201, 216, 231,
									 318, 342, 366));
}

y_test_func("Matrix determinant") {
	y_test_assert(Matrix3<>(1, 0, 0,
							7, 8, 0,
							4, 5, 6).determinant() == 48);
}

y_test_func("Matrix inverse") {
	auto i = Matrix3<>(1, 2, 3,
					   7, 8, 0,
					   4, 5, 6).inverse();
	auto e = Matrix3<>(-16, -1, 8,
					   14, 2, -7,
					   -1, -1, 2) * (1 / 9.0f);

	y_test_assert(e == i);
}


y_test_func("Matrix asymetrical") {
	Matrix<2, 3> mat(1, 2, 3,
					 4, 5, 6);

	y_test_assert(mat.row(0) == vec(1.0f, 2.0f, 3.0f) && mat.row(1) == vec(4, 5, 6));
	y_test_assert(mat.column(0) == vec(1, 4) && mat.column(1) == vec(2, 5) && mat.column(2) == vec(3, 6));
	y_test_assert(mat * vec(7.0f, 8.0f, 9.0f) == vec(50, 122));
	y_test_assert(mat.transposed() * vec(7.0f, 8.0f) == vec(39, 54, 69));
}

y_test_func("Matrix asymetrical multiply") {
	Matrix<2, 3> a(1, 2, 3,
				   4, 5, 6);

	Matrix<3, 2> b(7, 8,
				   9, 10,
				   11, 12);

	y_test_assert(a * b == matrix(vec(58, 64),
								  vec(139, 154)));

	y_test_assert((a * b).determinant() == 36);
}

y_test_func("Matrix sub") {
	Matrix<2, 3> mat(1, 2, 3,
					 4, 5, 6);

	using M = Matrix<1, 2>;
	y_test_assert(mat.sub(0, 2) == M(4, 5));
}
