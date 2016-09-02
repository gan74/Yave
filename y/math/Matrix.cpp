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

#include "Matrix.h"
#include <y/test/test.h>

namespace y {
namespace math {

static_assert(std::is_trivially_copyable<Matrix4<>>::value, "Matrix<T> should be trivially copyable");

y_test_func("Matrix vec multiply") {
	Matrix3<> mat(1, 2, 3,
				  4, 5, 6,
				  7, 8, 9);

	y_test_assert(mat * vec(10, 11, 12) == vec(68, 167, 266));
	y_test_assert(mat.transposed() * vec(10, 11, 12) == vec(138, 171, 204));
	y_test_assert(mat.transposed() * vec(10, 11, 12) == vec(138, 171, 204));
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

y_test_func("Matrix asymetrical") {
	Matrix<2, 3> mat(1, 2, 3,
					 4, 5, 6);

	y_test_assert(mat[0] == vec(1, 2, 3) && mat[1] == vec(4, 5, 6));
	y_test_assert(mat.column(0) == vec(1, 4) && mat.column(1) == vec(2, 5) && mat.column(2) == vec(3, 6));
	y_test_assert(mat * vec(7, 8, 9) == vec(50, 122));
	y_test_assert(mat.transposed() * vec(7, 8) == vec(39, 54, 69));
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

}
}
