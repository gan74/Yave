/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

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

#ifndef N_MATH_MATRIX_H
#define N_MATH_MATRIX_H

#include "Vec.h"

namespace n {
namespace math {

template<uint N, uint M, typename T>
class Matrix;

namespace internal {
	template<typename T, uint N>
	T determinant(const Matrix<N, N, T> &mat);

	template<typename T>
	T determinant(const Matrix<2, 2, T> &mat);

	template<typename T>
	T determinant(const Matrix<1, 1, T> &mat);
}


template<uint N, uint M, typename T = float> // N rows & M cols
class Matrix
{
	template<uint P, typename... Args>
	void build(T t, Args... args) {
		setAt(P, t);
		build<P + 1>(args...);
	}

	template<uint P, uint Q, typename U, typename... Args>
	void build(const Vec<Q, U> &t, Args... args) {
		for(uint i = 0; i != Q; i++) {
			setAt(P + i, T(t[i]));
		}
		build<P + Q>(args...);
	}

	template<uint P>
	void build() {
		static_assert(P == N * M, "Wrong number of arguments");
	}

	template<typename U>
	void setAt(uint i, U u) {
		rows[i / N][i % N] = u;
	}

	public:
		using iterator = T *;
		using const_iterator = const T *;

		Matrix() {
		}

		template<typename... Args>
		Matrix(T t, Args... args) {
			build<0>(t, args...);
		}

		template<uint P, typename... Args>
		Matrix(const Vec<P, T> &t, Args... args) {
			build<0>(t, args...);
		}

		Matrix(const Vec<N, T> r[N]) {
			for(uint i = 0; i != N; i++) {
				rows[i] = r[i];
			}
		}

		template<typename X>
		Matrix(const Matrix<N, M, X> &m) {
			for(uint i = 0; i != N; i++) {
				rows[i] = m[i];
			}
		}

		Vec<M, T> &operator[](uint i) {
			return rows[i];
		}

		const Vec<M, T> &operator[](uint i) const {
			return rows[i];
		}

		static constexpr bool isSquare() {
			return N == M;
		}

		template<typename U>
		Vec<M, U> operator*(const Vec<M, U> &v) const {
			Vec<M, U> tr;
			for(uint i = 0; i != M; i++) {
				tr += column(i) * v[i];
			}
			return tr;
		}

		template<typename U, uint P>
		auto operator*(const Matrix<M, P, U> &m) const -> Matrix<N, P, decltype(makeOne<T>() * makeOne<U>())> {
			Matrix<N, P, decltype(makeOne<T>() * makeOne<U>())> mat;
			for(uint i = 0; i != N; i++) {
				for(uint j = 0; j != P; j++) {
					decltype(makeOne<T>() * makeOne<U>()) tmp(0);
					for(uint k = 0; k != M; k++) {
						tmp = tmp + rows[i][k] * m[k][j];
					}
					mat[i][j] = tmp;
				}
			}
			return mat;
		}

		template<typename U>
		auto operator+(const Matrix<N, M, U> &m) const -> Matrix<N, M, decltype(makeOne<T>() + makeOne<U>())> {
			Matrix<N, M, decltype(makeOne<T>() * makeOne<U>())> mat;
			for(uint i = 0; i != N; i++) {
				for(uint j = 0; j != M; j++) {
					mat[i][j] = rows[i][j] + m[i][j];
				}
			}
			return mat;
		}

		Vec<N, T> column(uint col) const {
			Vec<N, T> c;
			for(uint i = 0; i != N; i++) {
				c[i] = rows[i][col];
			}
			return c;
		}

		Matrix<M, N, T> transposed() const {
			Matrix<M, N, T> tr;
			for(uint i = 0; i != N; i++) {
				for(uint j = 0; j != M; j++) {
					tr[j][i] = (*this)[i][j];
				}
			}
			return tr;
		}

		Matrix<N, M, T> &operator=(const Matrix<N, M, T> &m) {
			for(uint i = 0; i != N; i++) {
				for(uint j = 0; j != M; j++) {
					(*this)[i][j] = m[i][j];
				}
			}
			return *this;
		}

		bool operator==(const Matrix<N, M, T> &m) const {
			for(uint i = 0; i != N; i++) {
				for(uint j = 0; j != M; j++) {
					if((*this)[i][j] != m[i][j]) {
						return false;
					}
				}
			}
			return true;
		}

		Matrix<N - 1, M - 1, T> sub(uint r, uint c) const {
			Matrix<N - 1, M - 1, T> mat;
			for(uint i = 0; i != N - 1; i++) {
				for(uint j = 0; j != M - 1; j++) {
					uint ir = i < r ? i : i + 1;
					uint jr = j < c ? j : j + 1;
					mat[i][j] = (*this)[ir][jr];
				}
			}
			return mat;
		}

		T determinant() const {
			chksq();
			return internal::determinant(*this);
		}

		Matrix<N, M, T> inverse() const {
			Matrix<N, M, T> inv;
			T d = determinant();
			if(d == 0) {
				return Matrix<N, M, T>();
			}
			d = 1 / d;
			for(uint i = 0; i != N; i++) {
				for(uint j = 0; j != M; j++) {
					inv[j][i] = sub(i, j).determinant() * d * (i % 2 == j % 2 ? 1 : -1);
				}
			}
			return inv;
		}

		static constexpr Matrix<N, M, T> identity() {
			chksq();
			Matrix<N, M, T> mat;
			for(uint i = 0; i != N; i++) {
				mat[i][i] = T(1);
			}
			return mat;
		}

		const T *operator*() const {
			return &rows[0][0];
		}

		const_iterator begin() const {
			return &rows[0][0];
		}

		const_iterator end() const {
			return (&rows[0][0]) + (M * N);
		}

		T *operator*() {
			return &rows[0][0];
		}

		iterator begin() {
			return &rows[0][0];
		}

		iterator end() {
			return (&rows[0][0]) + (M * N);
		}

	private:
		static constexpr void chksq() {
			static_assert(isSquare(), "The matrix must be square");
		}

		Vec<M, T> rows[N];
};

namespace internal {
	template<typename T, uint N>
	T determinant(const Matrix<N, N, T> &mat) {
		struct {
			int operator()(int index) const {
				return 2 * (index % 2) - 1;
			}
		} sgn;
		T d(0);
		for(uint i = 0; i != N; i++) {
			d = d + sgn(i + 1) * mat[0][i] * mat.sub(0, i).determinant();
		}
		return d;
	}

	template<typename T>
	T determinant(const Matrix<2, 2, T> &mat) {
		return mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
	}


	template<typename T>
	T determinant(const Matrix<1, 1, T> &mat) {
		return mat[0][0];
	}
}

template<typename T = float>
using Matrix4 = Matrix<4, 4, T>;
template<typename T = float>
using Matrix3 = Matrix<3, 3, T>;
template<typename T = float>
using Matrix2 = Matrix<2, 2, T>;

}
}

#endif // N_MATH_MATRIX_H
