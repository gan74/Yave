/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include <algorithm>

namespace y {
namespace math {


template<usize N, usize M, typename T>
class Matrix;

namespace detail {
    template<typename T, usize N>
    inline constexpr T determinant(const Matrix<N, N, T>& mat);

    template<typename T>
    inline constexpr T determinant(const Matrix<2, 2, T>& mat);

    template<typename T>
    inline constexpr T determinant(const Matrix<1, 1, T>& mat);
}


template<usize N, usize M, typename T = float> // N rows & M cols
class Matrix {

    public:
        static constexpr usize vec_size = N;
        static constexpr usize vec_count = M;

        using Column = Vec<N, T>;
        using Row = Vec<M, T>;

    private:
        template<usize P, typename A, typename... Args, typename = std::enable_if_t<std::is_arithmetic_v<A>>>
        inline constexpr void build(A a, Args... args) {
            set_at(P, T(a));
            build<P + 1>(args...);
        }

        template<usize P, usize Q, typename... Args>
        inline constexpr void build(const Vec<Q, T>& t, Args... args) {
            for(usize i = 0; i != Q; ++i) {
                set_at(P + i, t[i]);
            }
            build<P + Q>(args...);
        }

        template<usize P>
        inline constexpr void build() {
            static_assert(P == N * M, "Wrong number of arguments");
        }

        template<typename U>
        inline constexpr void set_at(usize i, U u) {
            const usize r = i / M;
            const usize c = i % M;
            _vecs[c][r] = u;
        }

    public:
        using iterator = T*;
        using const_iterator = const T*;
        using value_type = T;

        inline constexpr Matrix() = default;
        inline constexpr Matrix(const Matrix&) = default;
        inline constexpr Matrix& operator=(const Matrix&) = default;

        template<typename U, typename... Args>
        inline constexpr explicit Matrix(U t, Args... args) {
            build<0>(t, args...);
        }

        template<typename X>
        inline constexpr Matrix(const Matrix<N, M, X>& m) {
            std::copy(m.begin(), m.end(), begin());
        }

        inline constexpr Matrix(detail::identity_t&&) : Matrix(Matrix::identity()) {
        }

        inline constexpr auto& operator[](usize i) {
            return _vecs[i];
        }

        inline constexpr const auto& operator[](usize i) const {
            return _vecs[i];
        }

        inline constexpr Row row(usize row) const {
            Row r;
            for(usize i = 0; i != M; ++i) {
                r[i] = _vecs[i][row];
            }
            return r;
        }

        inline constexpr Column& column(usize col) {
            return _vecs[col];
        }

        inline constexpr const Column& column(usize col) const {
            return _vecs[col];
        }

        inline static constexpr bool is_square() {
            return N == M;
        }

        template<typename U>
        inline constexpr auto operator+(const Matrix<N, M, U>& m) const {
            const Matrix<N, M, decltype(std::declval<T>() * std::declval<U>())> mat;
            for(usize i = 0; i != N; ++i) {
                mat._vecs[i] = _vecs[i] + m._vecs[i];
            }
            return mat;
        }

        inline constexpr Matrix<M, N, T> transposed() const {
            Matrix<M, N, T> tr;
            for(usize i = 0; i != vec_count; ++i) {
                for(usize j = 0; j != vec_size; ++j) {
                    tr._vecs[j][i] = _vecs[i][j];
                }
            }
            return tr;
        }

        inline constexpr bool operator==(const Matrix& m) const {
            for(usize i = 0; i != vec_count; ++i) {
                if(_vecs[i] != m._vecs[i]) {
                    return false;
                }
            }
            return true;
        }

        template<usize R, usize C>
        inline constexpr Matrix<R, C, T> to() const {
            static_assert(R <= N && C <= M, "Accessing out of bound member");
            Matrix<R, C, T> mat;
            for(usize i = 0; i != C; ++i) {
                mat._vecs[i] = _vecs[i].template to<R>();
            }
            return mat;
        }

        inline constexpr Matrix<N - 1, M - 1, T> sub(usize r, usize c) const {
            Matrix<N - 1, M - 1, T> mat;
            for(usize i = 0; i != vec_count - 1; ++i) {
                for(usize j = 0; j != vec_size - 1; ++j) {
                    const usize ir = i < c ? i : i + 1;
                    const usize jr = j < r ? j : j + 1;
                    mat._vecs[i][j] = _vecs[ir][jr];
                }
            }
            return mat;
        }

        inline constexpr Matrix abs() const {
            Matrix mat;
            for(usize i = 0; i != vec_count; ++i) {
                mat[i] = _vecs[i].abs();
            }
            return mat;
        }

        inline constexpr T determinant() const {
            chk_sq();
            return detail::determinant(*this);
        }

        inline constexpr Matrix inverse() const {
            T d = determinant();
            if(d == 0) {
                return Matrix();
            }
            Matrix inv;
            d = 1 / d;
            for(usize i = 0; i != N; ++i) {
                for(usize j = 0; j != N; ++j) {
                    const auto s = sub(i, j).determinant() * d * (i % 2 == j % 2 ? 1 : -1);
                    inv._vecs[i][j] = s;
                }
            }
            return inv;
        }

        inline static constexpr Matrix identity() {
            chk_sq();
            Matrix mat;
            for(usize i = 0; i != N; ++i) {
                mat._vecs[i][i] = T(1);
            }
            return mat;
        }

        inline constexpr const_iterator begin() const {
            return &_vecs[0][0];
        }

        inline constexpr const_iterator end() const {
            return (&_vecs[0][0]) + (M * N);
        }

        inline constexpr iterator begin() {
            return& _vecs[0][0];
        }

        inline constexpr iterator end() {
            return (&_vecs[0][0]) + (M * N);
        }

        inline constexpr usize size() const {
            return N * M;
        }

        inline constexpr Column operator*(const Row& v) const {
            Column tr;
            for(usize i = 0; i != M; ++i) {
                tr += column(i) * v[i];
            }
            return tr;
        }

        template<typename U, usize P>
        inline constexpr auto operator*(const Matrix<M, P, U>& m) const {
            Matrix<N, P, decltype(std::declval<T>() * std::declval<U>())> mat;
            for(usize i = 0; i != N; ++i) {
                for(usize j = 0; j != P; ++j) {
                    decltype(std::declval<T>() * std::declval<U>()) tmp(0);
                    for(usize k = 0; k != M; ++k) {
                        tmp = tmp + _vecs[k][i] * m._vecs[j][k];
                    }
                    mat._vecs[j][i] = tmp;
                }
            }
            return mat;
        }

        template<typename U, usize P>
        inline constexpr Matrix& operator*=(const Matrix<M, P, U>& m) {
            return operator=(*this * m);
        }

        inline constexpr Matrix& operator*=(const T& t) {
            for(auto& i : *this) {
                i *= t;
            }
            return *this;
        }

    private:
        template<usize X, usize Y, typename U>
        friend class Matrix;

        inline static constexpr void chk_sq() {
            static_assert(is_square(), "The matrix must be square");
        }

        Vec<vec_size, T> _vecs[vec_count] = {};
};

namespace detail {
    template<typename T, usize N>
    constexpr T determinant(const Matrix<N, N, T>& mat) {
        auto sgn = [](usize index) {
            return 2 * int(index % 2) - 1;
        };
        T d(0);
        const auto row = mat.row(0);
        for(usize i = 0; i != N; ++i) {
            d = d + sgn(i + 1) * row[i] * mat.sub(0, i).determinant();
        }
        return d;
    }

    template<typename T>
    constexpr T determinant(const Matrix<2, 2, T>& mat) {
        return mat[0][0] * mat[1][1] - mat[0][1] * mat[1][0];
    }


    template<typename T>
    constexpr T determinant(const Matrix<1, 1, T>& mat) {
        return mat[0][0];
    }
}


template<typename T = float>
using Matrix4 = Matrix<4, 4, T>;
template<typename T = float>
using Matrix3 = Matrix<3, 3, T>;
template<typename T = float>
using Matrix2 = Matrix<2, 2, T>;


template<usize N, usize M, typename T>
inline constexpr auto operator*(Matrix<N, M, T> mat, const T& r) {
    return mat *= r;
}

template<usize N, usize M, typename T>
inline constexpr auto operator*(const T& r, Matrix<N, M, T> mat) {
    return mat *= r;
}


static_assert(std::is_trivially_copyable_v<Matrix4<>>, "Matrix<T> should be trivially copyable");

}
}

#endif // Y_MATH_MATRIX_H

