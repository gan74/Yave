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
#ifndef Y_MATH_VEC_H
#define Y_MATH_VEC_H

#include <y/utils.h>

#include <cmath>

namespace y {
namespace math {

namespace detail {
struct identity_t : NonCopyable {
    inline constexpr identity_t() {
    }

    inline identity_t(identity_t&&) {
    }
};
}

inline auto identity() {
    return detail::identity_t();
}

template<usize N, typename T = float>
class Vec
{
    template<usize P, typename A, typename... Args, typename = std::enable_if_t<std::is_arithmetic_v<A>>>
    inline constexpr void build(A a, Args... args) {
        _vec[P] = T(a);
        build<P + 1>(args...);
    }

    template<usize P, usize Q, typename U, typename... Args>
    inline constexpr void build(const Vec<Q, U>& t, Args... args) {
        for(usize i = 0; i != Q; ++i) {
            _vec[P + i] = T(t[i]);
        }
        build<P + Q>(args...);
    }

    template<usize P>
    inline constexpr void build() {
        static_assert(P == N, "Wrong number of arguments");
    }


    static_assert(N != 0, "Invalid size for Vec");
    static_assert(std::is_arithmetic_v<T>, "Invalid type <T> for Vec");


    public:
        using value_type = typename std::remove_const_t<T>;
        using iterator = T*;
        using const_iterator = const T*;

        template<typename A, typename B, typename... Args>
        inline constexpr Vec(A a, B b, Args... args) {
            build<0>(a, b, args...);
        }

        inline explicit constexpr Vec(T t) {
            for(usize i = 0; i != N; ++i) {
                _vec[i] = t;
            }
        }

        template<typename X>
        inline constexpr Vec(const Vec<N, X>& v) {
            for(usize i = 0; i != N; ++i) {
                _vec[i] = T(v[i]);
            }
        }

        template<usize S>
        inline constexpr Vec(const T(&v)[S]) {
            static_assert(S == N || !S, "Wrong number of arguments");
            for(usize i = 0; i != N; ++i) {
                _vec[i] = v[i];
            }
        }

        inline constexpr Vec(detail::identity_t&&) {
        }

        inline constexpr Vec() = default;
        inline constexpr Vec(const Vec&) = default;
        inline constexpr Vec& operator=(const Vec&) = default;

        inline constexpr T length2() const {
            return dot(*this);
        }

        inline constexpr auto length() const {
            return std::sqrt(length2());
        }

        inline constexpr T dot(const Vec& o) const {
            T sum = 0;
            for(usize i = 0; i != N; ++i) {
                sum += _vec[i] * o._vec[i];
            }
            return sum;
        }

        inline constexpr Vec cross(const Vec& o) const {
            Vec v;
            for(usize i = 0; i != N; ++i) {
                v[i] = _vec[(i + 1) % N] * o._vec[(i + 2) % N] - _vec[(i + 2) % N] * o._vec[(i + 1) % N];
            }
            return v;
        }

        inline constexpr void normalize() {
            if(!is_zero()) {
                operator*=(1.0f / length());
            }
        }

        inline constexpr Vec normalized() const {
            Vec v(*this);
            v.normalize();
            return v;
        }

        inline constexpr Vec abs() const {
            static_assert(std::is_signed_v<T>, "Vec<T>::abs makes no sense for T unsigned");
            Vec v;
            for(usize i = 0; i != N; ++i) {
                v[i] = _vec[i] < 0 ? -_vec[i] : _vec[i];
            }
            return v;
        }

        inline constexpr Vec saturated() const {
            Vec v;
            for(usize i = 0; i != N; ++i) {
                v[i] = std::min(std::max(_vec[i], T(0)), T(1));
            }
            return v;
        }

        inline constexpr Vec max(const Vec& v) const {
            Vec m;
            for(usize i = 0; i != N; ++i) {
                m[i] = std::max(_vec[i], v[i]);
            }
            return m;
        }

        inline constexpr Vec min(const Vec& v) const {
            Vec m;
            for(usize i = 0; i != N; ++i) {
                m[i] = std::min(_vec[i], v[i]);
            }
            return m;
        }

        inline constexpr T& x() {
            return _vec[0];
        }

        inline constexpr const T& x() const {
            return _vec[0];
        }

        inline constexpr T& y() {
            static_assert(N > 1, "Accessing out of bound member");
            return _vec[1];
        }

        inline constexpr const T& y() const {
            static_assert(N > 1, "Accessing out of bound member");
            return _vec[1];
        }

        inline constexpr T& z() {
            static_assert(N > 2, "Accessing out of bound member");
            return _vec[2];
        }

        inline constexpr const T& z() const {
            static_assert(N > 2, "Accessing out of bound member");
            return _vec[2];
        }

        inline constexpr T& w() {
            static_assert(N > 3, "Accessing out of bound member");
            return _vec[3];
        }

        inline constexpr const T& w() const {
            static_assert(N > 3, "Accessing out of bound member");
            return _vec[3];
        }

        template<usize M>
        inline constexpr const Vec<M, T>& to() const {
            static_assert(M <= N, "Accessing out of bound member");
            return reinterpret_cast<const Vec<M, T>&>(*this);
        }

        template<usize M>
        inline constexpr Vec<M, T>& to() {
            static_assert(M <= N, "Accessing out of bound member");
            return reinterpret_cast<Vec<M, T>&>(*this);
        }

        inline constexpr Vec<N - 1, T> sub(usize m) const {
            Vec<N - 1, T> v;
            for(usize i = 0; i != m; ++i) {
                v[i] = _vec[i];
            }
            for(usize i = m; i < N - 1; ++i) {
                v[i] = _vec[i + 1];
            }
            return v;
        }

        inline constexpr bool is_zero() const {
            for(usize i = 0; i != N; ++i) {
                if(_vec[i]) {
                    return false;
                }
            }
            return true;
        }

        inline constexpr const_iterator begin() const {
            return _vec;
        }

        inline constexpr const_iterator end() const {
            return _vec + N;
        }

        inline constexpr const_iterator cbegin() const {
            return _vec;
        }

        inline constexpr const_iterator cend() const {
            return _vec + N;
        }

        inline constexpr iterator begin() {
            return _vec;
        }

        inline constexpr iterator end() {
            return _vec + N;
        }

        inline constexpr T* data() {
            return _vec;
        }

        inline constexpr const T* data() const {
            return _vec;
        }

        inline constexpr T& operator[](usize i) {
            return _vec[i];
        }

        inline constexpr const T& operator[](usize i) const {
            return _vec[i];
        }

        inline static constexpr usize size() {
            return N;
        }

        template<usize I>
        inline constexpr const T& get() const {
            static_assert(I < N, "Accessing out of bound member");
            return _vec[I];
        }

        template<usize I>
        inline constexpr void set(const T& t) {
            static_assert(I < N, "Accessing out of bound member");
            _vec[I] = t;
        }


        inline constexpr bool operator!=(const Vec<N, T>& o) const {
            for(usize i = 0; i != N; ++i) {
                if(o._vec[i] != _vec[i]) {
                    return true;
                }
            }
            return false;
        }

        inline constexpr bool operator==(const Vec<N, T>& o) const {
            return !operator!=(o);
        }

        inline constexpr Vec operator-() const {
            Vec t;
            for(usize i = 0; i != N; ++i) {
                t[i] = -_vec[i];
            }
            return t;
        }

        inline constexpr Vec& operator*=(const T& t) {
            for(usize i = 0; i != N; ++i) {
                _vec[i] *= t;
            }
            return *this;
        }

        inline constexpr Vec& operator/=(const T& t) {
            for(usize i = 0; i != N; ++i) {
                _vec[i] /= t;
            }
            return *this;
        }

        inline constexpr Vec& operator+=(const T& t) {
            for(usize i = 0; i != N; ++i) {
                _vec[i] += t;
            }
            return *this;
        }

        inline constexpr Vec& operator-=(const T& t) {
            for(usize i = 0; i != N; ++i) {
                _vec[i] -= t;
            }
            return *this;
        }



        inline constexpr Vec& operator*=(const Vec& v) {
            for(usize i = 0; i != N; ++i) {
                _vec[i] *= v[i];
            }
            return *this;
        }

        inline constexpr Vec& operator/=(const Vec& v) {
            for(usize i = 0; i != N; ++i) {
                _vec[i] /= v[i];
            }
            return *this;
        }

        inline constexpr Vec& operator+=(const Vec& v) {
            for(usize i = 0; i != N; ++i) {
                _vec[i] += v[i];
            }
            return *this;
        }

        inline constexpr Vec& operator-=(const Vec& v) {
            for(usize i = 0; i != N; ++i) {
                _vec[i] -= v[i];
            }
            return *this;
        }


        inline constexpr Vec& operator=(const T& v) {
            for(usize i = 0; i != N; ++i) {
                _vec[i] = v;
            }
            return *this;
        }

    private:
        template<usize M, typename U>
        friend class Vec;

        T _vec[N] = {T(0)};

};




using Vec2 = Vec<2>;
using Vec3 = Vec<3>;
using Vec4 = Vec<4>;

using Vec2d = Vec<2, double>;
using Vec3d = Vec<3, double>;
using Vec4d = Vec<4, double>;

using Vec2i = Vec<2, i32>;
using Vec3i = Vec<3, i32>;
using Vec4i = Vec<4, i32>;

using Vec2ui = Vec<2, u32>;
using Vec3ui = Vec<3, u32>;
using Vec4ui = Vec<4, u32>;

using Vec2b = Vec<2, u8>;
using Vec3b = Vec<3, u8>;
using Vec4b = Vec<4, u8>;




namespace detail {

template<usize N, typename A, typename B>
struct V {
    using type = Vec<N, typename std::common_type_t<A, B>>;
};

}


template<usize N, typename T, typename R>
auto operator+(const Vec<N, T>& v, const R& r) {
    typename detail::V<N, T, R>::type vec(v);
    vec += r;
    return vec;
}

template<usize N, typename T, typename L>
auto operator+(const L& l, const Vec<N, T>& v) {
    return v + l;
}

template<usize N, typename T>
auto operator+(Vec<N, T> a, const Vec<N, T>& b) {
    a += b;
    return a;
}




template<usize N, typename T, typename R>
auto operator*(const Vec<N, T>& v, const R& r) {
    typename detail::V<N, T, R>::type vec(v);
    vec *= r;
    return vec;
}

template<usize N, typename T, typename L>
auto operator*(const L& l, const Vec<N, T>& v) {
    return v * l;
}

template<usize N, typename T>
auto operator*(Vec<N, T> a, const Vec<N, T>& b) {
    a *= b;
    return a;
}




template<usize N, typename T, typename R>
auto operator-(const Vec<N, T>& v, const R& r) {
    typename detail::V<N, T, R>::type vec(v);
    vec -= r;
    return vec;
}

template<usize N, typename T, typename L>
auto operator-(const L& l, const Vec<N, T>& v) {
    return -v + l;
}

template<usize N, typename T>
auto operator-(Vec<N, T> a, const Vec<N, T>& b) {
    a -= b;
    return a;
}



template<usize N, typename T, typename R>
auto operator/(const Vec<N, T>& v, const R& r) {
    typename detail::V<N, T, R>::type vec(v);
    vec /= r;
    return vec;
}

template<usize N, typename T, typename L>
auto operator/(const L& l, const Vec<N, T>& v) {
    typename detail::V<N, T, L>::type vec(l);
    return vec / v;
}

template<usize N, typename T>
auto operator/(Vec<N, T> a, const Vec<N, T>& b) {
    a /= b;
    return a;
}


static_assert(std::is_trivially_copyable_v<Vec4>, "Vec<T> should be trivially copyable");

}
}


#endif // Y_MATH_VEC_H

