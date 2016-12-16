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

#ifndef Y_MATH_MATRIX_H
#define Y_MATH_MATRIX_H

#include <y/utils.h>
#include "Vec.h"

namespace y {
namespace math {

template<usize N, usize M, typename T>
class Matrix;

namespace detail {
	template<typename T, usize N>
	T determinant(const Matrix<N, N, T>& mat);

	template<typename T>
	T determinant(const Matrix<2, 2, T>& mat);

	template<typename T>
	T determinant(const Matrix<1, 1, T>& mat);
}


template<usize N, usize M, typename T = float> // N rows & M cols
class Matrix
{
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
		_rows[i / M][i % M] = u;
	}

	public:
		using iterator = T*;
		using const_iterator = const T*;

		template<typename U, typename... Args>
		Matrix(U t, Args... args) {
			build<0>(t, args...);
		}


		Matrix(const Vec<M, T> r[N]) {
			for(usize i = 0; i != N; i++) {
				_rows[i] = r[i];
			}
		}

		template<typename X>
		Matrix(const Matrix<N, M, X>& m) {
			for(usize i = 0; i != N; i++) {
				_rows[i] = m[i];
			}
		}

		Matrix(detail::identity_t&&) : Matrix(Matrix::identity()) {
		}

		Matrix() = default;
		Matrix(const Matrix&) = default;
		Matrix& operator=(const Matrix&) = default;

		Vec<M, T>& operator[](usize i) {
			return _rows[i];
		}

		const Vec<M, T>& operator[](usize i) const {
			return _rows[i];
		}

		static constexpr bool is_square() {
			return N == M;
		}

		template<typename U>
		Vec<N, U> operator*(const Vec<M, U>& v) const {
			Vec<N, U> tr;
			for(usize i = 0; i != M; i++) {
				tr += column(i) * v[i];
			}
			return tr;
		}

		template<typename U, usize P>
		auto operator*(const Matrix<M, P, U>& m) const {
			Matrix<N, P, decltype(make_one<T>() * make_one<U>())> mat;
			for(usize i = 0; i != N; i++) {
				for(usize j = 0; j != P; j++) {
					decltype(make_one<T>() * make_one<U>()) tmp(0);
					for(usize k = 0; k != M; k++) {
						tmp = tmp + _rows[i][k] * m[k][j];
					}
					mat[i][j] = tmp;
				}
			}
			return mat;
		}

		template<typename U>
		auto operator+(const Matrix<N, M, U>& m) const {
			Matrix<N, M, decltype(make_one<T>() * make_one<U>())> mat;
			for(usize i = 0; i != N; i++) {
				for(usize j = 0; j != M; j++) {
					mat[i][j] = _rows[i][j] + m[i][j];
				}
			}
			return mat;
		}

		Vec<N, T> column(usize col) const {
			Vec<N, T> c;
			for(usize i = 0; i != N; i++) {
				c[i] = _rows[i][col];
			}
			return c;
		}

		Matrix<M, N, T> transposed() const {
			Matrix<M, N, T> tr;
			for(usize i = 0; i != N; i++) {
				for(usize j = 0; j != M; j++) {
					tr[j][i] = (*this)[i][j];
				}
			}
			return tr;
		}

		bool operator==(const Matrix& m) const {
			for(usize i = 0; i != N; i++) {
				for(usize j = 0; j != M; j++) {
					if((*this)[i][j] != m[i][j]) {
						return false;
					}
				}
			}
			return true;
		}

		Matrix<N - 1, M - 1, T> sub(usize r, usize c) const {
			Matrix<N - 1, M - 1, T> mat;
			for(usize i = 0; i != N - 1; i++) {
				for(usize j = 0; j != M - 1; j++) {
					usize ir = i < r ? i : i + 1;
					usize jr = j < c ? j : j + 1;
					mat[i][j] = (*this)[ir][jr];
				}
			}
			return mat;
		}

		T determinant() const {
			chk_sq();
			return detail::determinant(*this);
		}

		Matrix inverse() const {
			Matrix inv;
			T d = determinant();
			if(d == 0) {
				return Matrix();
			}
			d = 1 / d;
			for(usize i = 0; i != N; i++) {
				for(usize j = 0; j != M; j++) {
					inv[j][i] = sub(i, j).determinant() * d * (i % 2 == j % 2 ? 1 : -1);
				}
			}
			return inv;
		}

		static constexpr Matrix identity() {
			chk_sq();
			Matrix mat;
			for(usize i = 0; i != N; i++) {
				mat[i][i] = T(1);
			}
			return mat;
		}

		const_iterator begin() const {
			return &_rows[0][0];
		}

		const_iterator end() const {
			return (&_rows[0][0]) + (M * N);
		}

		iterator begin() {
			return& _rows[0][0];
		}

		iterator end() {
			return (&_rows[0][0]) + (M * N);
		}

	private:
		static constexpr void chk_sq() {
			static_assert(is_square(), "The matrix must be square");
		}

		Vec<M, T> _rows[N] = {Vec<M, T>()};
};

namespace detail {
	template<typename T, usize N>
	T determinant(const Matrix<N, N, T>& mat) {
		struct {
			int operator()(int index) const {
				return 2 * (index % 2) - 1;
			}
		} sgn;
		T d(0);
		for(usize i = 0; i != N; i++) {
			d = d + sgn(i + 1) * mat[0][i] * mat.sub(0, i).determinant();
		}
		return d;
	}

	template<typename T>
	T determinant(const Matrix<2, 2, T>& mat) {
		return mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
	}


	template<typename T>
	T determinant(const Matrix<1, 1, T>& mat) {
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

}
}

#endif // Y_MATH_MATRIX_H
