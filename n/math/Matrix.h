/*******************************
Copyright (C) 2013-2014 gr�goire ANGERAND

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

template<typename T, uint N, uint M>
class Matrix;

namespace internal {
	template<typename T, uint N>
	T determinant(const Matrix<T, N, N> &mat);

	template<typename T>
	T determinant(const Matrix<T, 2, 2> &mat);

	template<typename T>
	T determinant(const Matrix<T, 1, 1> &mat);
}


template<typename T, uint N, uint M> // N rows & M cols
class Matrix
{
	template<uint P, typename... Args>
	void build(T t, Args... args) {
		setAt(P, t);
		build<P + 1>(args...);
	}

	template<uint P, uint Q, typename U, typename... Args>
	void build(const Vec<U, Q> &t, Args... args) {
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
		Matrix() {
		}

		template<typename... Args>
		Matrix(T t, Args... args) {
			build<0>(t, args...);
		}

		Matrix(const Vec<T, N> r[M]) {
			for(uint i = 0; i != M; i++) {
				rows[i] = r[i];
			}
		}

		Vec<T, M> &operator[](uint i) {
			return rows[i];
		}

		const Vec<T, M> &operator[](uint i) const {
			return rows[i];
		}

		static constexpr bool isSquare() {
			return N == M;
		}

		template<typename U, uint P>
		auto operator*(const Matrix<U, M, P> &m) const -> Matrix<decltype(makeOne<T>() * makeOne<U>()), N, P> {
			Matrix<decltype(makeOne<T>() * makeOne<U>()), N, P> mat;
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
		auto operator+(const Matrix<U, N, M> &m) const -> Matrix<decltype(makeOne<T>() + makeOne<U>()), N, M> {
			Matrix<decltype(makeOne<T>() * makeOne<U>()), N, M> mat;
			for(uint i = 0; i != N; i++) {
				for(uint j = 0; j != M; j++) {
					mat[i][j] = rows[i][j] + m[i][j];
				}
			}
			return mat;
		}

		Vec<T, N> column(uint col) const {
			Vec<T, N> c;
			for(uint i = 0; i != N; i++) {
				c[i] = rows[i][col];
			}
		}

		Matrix<T, M, N> transposed() const {
			Matrix<T, M, N> tr;
			for(uint i = 0; i != N; i++) {
				for(uint j = 0; j != M; i++) {
					tr[j][i] = (*this)[i][j];
				}
			}
			return tr;
		}

		Matrix<T, N, M> &operator=(const Matrix<T, N, M> &m) {
			for(uint i = 0; i != N; i++) {
				for(uint j = 0; j != M; j++) {
					(*this)[i][j] = m[i][j];
				}
			}
			return *this;
		}

		bool operator==(const Matrix<T, N, M> &m) const {
			for(uint i = 0; i != N; i++) {
				for(uint j = 0; j != M; j++) {
					if((*this)[i][j] != m[i][j]) {
						return false;
					}
				}
			}
			return true;
		}

		Matrix<T, N - 1, M - 1> sub(uint r, uint c) const {
			Matrix<T, N - 1, M - 1> mat;
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

		Matrix<T, N, M> inverse() const {
			Matrix<T, N, M> inv;
			T d = determinant();
			if(d == 0) {
				return Matrix<T, N, M>();
			}
			d = 1 / d;
			for(uint i = 0; i != N; i++) {
				for(uint j = 0; j != M; j++) {
					inv[j][i] = sub(i, j).determinant() * d * (i % 2 == j % 2 ? 1 : -1);
				}
			}
			return inv;
		}

		static constexpr Matrix<T, N, M> identity() {
			chksq();
			Matrix<T, N, M> mat;
			for(uint i = 0; i != N; i++) {
				mat[i][i] = T(1);
			}
			return mat;
		}

	private:
		static constexpr void chksq() {
			static_assert(isSquare(), "The matrix must be square");
		}

		Vec<T, M> rows[N];
};

namespace internal {
	template<typename T, uint N>
	T determinant(const Matrix<T, N, N> &mat) {
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
	T determinant(const Matrix<T, 2, 2> &mat) {
		return mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
	}


	template<typename T>
	T determinant(const Matrix<T, 1, 1> &mat) {
		return mat[0][0];
	}
}

}
}

#endif // N_MATH_MATRIX_H
