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

#ifndef Y_MATH_MATRIX_H
#define Y_MATH_MATRIX_H

#include <y/utils.h>
#include "Vec.h"

namespace y {
namespace math {

enum class MatrixLayout {
	RowMajor,
	ColumnMajor
};

template<usize N, usize M, typename T, MatrixLayout L = MatrixLayout::ColumnMajor>
class Matrix;

namespace detail {
	template<typename T, usize N, MatrixLayout L>
	T determinant(const Matrix<N, N, T, L>& mat);

	template<typename T, MatrixLayout L>
	T determinant(const Matrix<2, 2, T, L>& mat);

	template<typename T, MatrixLayout L>
	T determinant(const Matrix<1, 1, T, L>& mat);
}


template<usize N, usize M, typename T = float, MatrixLayout Layout> // N rows & M cols
class Matrix {

	public:
		static constexpr bool is_row_major = Layout == MatrixLayout::RowMajor;
		static constexpr bool is_column_major = !is_row_major;
		using is_row_major_t = bool_type<is_row_major>;

		static constexpr usize vec_size = is_row_major ? M : N;
		static constexpr usize vec_count = is_row_major ? N : M;

		using Column = Vec<N, T>;
		using Row = Vec<M, T>;

	private:
		template<usize P, typename... Args>
		void build(T t, Args... args) {
			set_at(P, t);
			build<P + 1>(args...);
		}

		template<usize P, usize Q, typename U, typename... Args>
		void build(const Vec<Q, U>& t, Args... args) {
			for(usize i = 0; i != Q; i++) {
				set_at(P + i, T(t[i]));
			}
			build<P + Q>(args...);
		}

		template<usize P>
		void build() {
			static_assert(P == N * M, "Wrong number of arguments");
		}

		template<typename U>
		void set_at(usize i, U u) {
			usize r = i / M;
			usize c = i % M;
			if(is_row_major) {
				_vecs[r][c] = u;
			} else {
				_vecs[c][r] = u;
			}
		}

		using Row_cr = std::conditional_t<is_row_major, const Row&, Row>;
		using Row_r = std::conditional_t<is_row_major, Row&, Row>;
		using Column_cr = std::conditional_t<is_row_major, Column, const Column&>;
		using Column_r = std::conditional_t<is_row_major, Column, Column&>;

	public:
		using iterator = T*;
		using const_iterator = const T*;

		template<typename U, typename... Args>
		Matrix(U t, Args... args) {
			build<0>(t, args...);
		}



		template<typename X>
		Matrix(const Matrix<N, M, X>& m) {
			std::copy(m.begin(), m.end(), begin());
		}

		Matrix(detail::identity_t&&) : Matrix(Matrix::identity()) {
		}

		Matrix() = default;
		Matrix(const Matrix&) = default;
		Matrix& operator=(const Matrix&) = default;


		auto& operator[](usize i) {
			return _vecs[i];
		}

		const auto& operator[](usize i) const {
			return _vecs[i];
		}

		template<typename RM = is_row_major_t, typename = std::enable_if_t<RM::value>>
		Row_r row(usize i) {
			return row(i, is_row_major_t());
		}

		Row_cr row(usize i) const {
			return row(i, is_row_major_t());
		}

		template<typename RM = is_row_major_t, typename = std::enable_if_t<!RM::value>>
		Column_r column(usize col) {
			return column(col, is_row_major_t());
		}

		Column_cr column(usize col) const {
			return column(col, is_row_major_t());
		}


		static constexpr bool is_square() {
			return N == M;
		}

		template<typename U>
		auto operator+(const Matrix<N, M, U, Layout>& m) const {
			Matrix<N, M, decltype(std::declval<T>() * std::declval<U>())> mat;
			for(usize i = 0; i != N; i++) {
				for(usize j = 0; j != M; j++) {
					mat.vecs[i][j] = _vecs[i][j] + m._vecs[i][j];
				}
			}
			return mat;
		}

		Matrix<M, N, T, Layout> transposed() const {
			Matrix<M, N, T, Layout> tr;
			for(usize i = 0; i != vec_count; i++) {
				for(usize j = 0; j != vec_size; j++) {
					tr._vecs[j][i] = _vecs[i][j];
				}
			}
			return tr;
		}

		bool operator==(const Matrix& m) const {
			for(usize i = 0; i != vec_count; i++) {
				for(usize j = 0; j != vec_size; j++) {
					if(_vecs[i][j] != m._vecs[i][j]) {
						return false;
					}
				}
			}
			return true;
		}

		Matrix<N - 1, M - 1, T, Layout> sub(usize r, usize c) const {
			if(!is_row_major) {
				std::swap(r, c);
			}
			Matrix<N - 1, M - 1, T,Layout> mat;
			for(usize i = 0; i != vec_count - 1; i++) {
				for(usize j = 0; j != vec_size - 1; j++) {
					usize ir = i < r ? i : i + 1;
					usize jr = j < c ? j : j + 1;
					mat._vecs[i][j] = _vecs[ir][jr];
				}
			}
			return mat;
		}

		T determinant() const {
			chk_sq();
			return detail::determinant(*this);
		}

		Matrix inverse() const {
			T d = determinant();
			if(d == 0) {
				return Matrix();
			}
			Matrix inv;
			d = 1 / d;
			for(usize i = 0; i != N; i++) {
				for(usize j = 0; j != N; j++) {
					auto s = sub(i, j).determinant() * d * (i % 2 == j % 2 ? 1 : -1);
					(is_row_major ? inv._vecs[j][i] : inv._vecs[i][j]) = s;
				}
			}
			return inv;
		}

		static constexpr Matrix identity() {
			chk_sq();
			Matrix mat;
			for(usize i = 0; i != N; i++) {
				mat._vecs[i][i] = T(1);
			}
			return mat;
		}

		const_iterator begin() const {
			return &_vecs[0][0];
		}

		const_iterator end() const {
			return (&_vecs[0][0]) + (M * N);
		}

		iterator begin() {
			return& _vecs[0][0];
		}

		iterator end() {
			return (&_vecs[0][0]) + (M * N);
		}

		Column operator*(const Row& v) const {
			Column tr;
			for(usize i = 0; i != M; i++) {
				tr += column(i) * v[i];
			}
			return tr;
		}

		template<typename U, usize P>
		auto operator*(const Matrix<M, P, U, Layout>& m) const {
			Matrix<N, P, decltype(std::declval<T>() * std::declval<U>())> mat;
			for(usize i = 0; i != N; i++) {
				for(usize j = 0; j != P; j++) {
					decltype(std::declval<T>() * std::declval<U>()) tmp(0);
					for(usize k = 0; k != M; k++) {
						tmp = tmp + row(i)[k] * m.row(k)[j];
					}
					if(is_row_major) {
						mat._vecs[i][j] = tmp;
					} else {
						mat._vecs[j][i] = tmp;
					}
				}
			}
			return mat;
		}

		template<typename U, usize P>
		Matrix& operator*=(const Matrix<M, P, U, Layout>& m) {
			return operator=(*this * m);
		}

		Matrix& operator*=(const T& t) {
			for(auto& i : *this) {
				i *= t;
			}
			return *this;
		}

	private:
		template<usize X, usize Y, typename U, MatrixLayout L>
		friend class Matrix;

		static constexpr void chk_sq() {
			static_assert(is_square(), "The matrix must be square");
		}

		Row_cr row(usize i, std::true_type) const {
			return _vecs[i];
		}

		template<typename RM = is_row_major_t, typename = std::enable_if_t<RM::value>>
		Row_r row(usize i, std::true_type) {
			return _vecs[i];
		}

		Column column(usize col, std::true_type) const {
			Column c;
			for(usize i = 0; i != N; i++) {
				c[i] = _vecs[i][col];
			}
			return c;
		}

		Row row(usize row, std::false_type) const {
			Row r;
			for(usize i = 0; i != M; i++) {
				r[i] = _vecs[i][row];
			}
			return r;
		}

		Column_cr column(usize i, std::false_type) const {
			return _vecs[i];
		}

		template<typename RM = is_row_major_t, typename = std::enable_if_t<!RM::value>>
		Column_r column(usize i, std::false_type) {
			return _vecs[i];
		}


		Vec<vec_size, T> _vecs[vec_count] = {};
};

namespace detail {
	template<typename T, usize N, MatrixLayout L>
	T determinant(const Matrix<N, N, T, L>& mat) {
		struct {
			int operator()(int index) const {
				return 2 * (index % 2) - 1;
			}
		} sgn;
		T d(0);
		auto row = mat.row(0);
		for(usize i = 0; i != N; i++) {
			d = d + sgn(i + 1) * row[i] * mat.sub(0, i).determinant();
		}
		return d;
	}

	template<typename T, MatrixLayout L>
	T determinant(const Matrix<2, 2, T, L>& mat) {
		return mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
	}


	template<typename T, MatrixLayout L>
	T determinant(const Matrix<1, 1, T, L>& mat) {
		return mat[0][0];
	}
}


template<typename T = float>
using Matrix4 = Matrix<4, 4, T>;
template<typename T = float>
using Matrix3 = Matrix<3, 3, T>;
template<typename T = float>
using Matrix2 = Matrix<2, 2, T>;


template<usize M, typename T, typename... Args>
auto matrix(const Vec<M, T>& v, Args... args) {
	return Matrix<sizeof...(args) + 1, M, T>(v, args...);
}

template<usize N, usize M, typename T, MatrixLayout L>
auto operator*(Matrix<N, M, T, L> mat, const T& r) {
	return mat *= r;
}

template<usize N, usize M, typename T, MatrixLayout L>
auto operator*(const T& r, Matrix<N, M, T, L> mat) {
	return mat *= r;
}


static_assert(std::is_trivially_copyable<Matrix4<>>::value, "Matrix<T> should be trivially copyable");

}
}

#endif // Y_MATH_MATRIX_H
