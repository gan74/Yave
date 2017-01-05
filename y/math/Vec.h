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

#ifndef Y_MATH_VEC_H
#define Y_MATH_VEC_H

#include <y/utils.h>
#include <cmath>

namespace y {
namespace math {

namespace detail {
struct identity_t : NonCopyable {
	constexpr identity_t() {
	}

	identity_t(identity_t &&) {
	}
};
}

inline auto identity() {
	return detail::identity_t();
}

template<usize N, typename T = float>
class Vec
{
	template<usize P, typename... Args>
	void build(T t, Args... args) {
		_vec[P] = t;
		build<P + 1>(args...);
	}

	template<usize P, usize Q, typename U, typename... Args>
	void build(const Vec<Q, U>& t, Args... args) {
		for(usize i = 0; i != Q; i++) {
			_vec[P + i] = t[i];
		}
		build<P + Q>(args...);
	}

	template<usize P>
	void build() {
		static_assert(P == N, "Wrong number of arguments");
	}


	static_assert(N != 0, "Invalid size for Vec");
	static_assert(std::is_arithmetic<T>::value, "Invalid type <T> for Vec");


	public:
		using Element = typename std::remove_const<T>::type;
		using iterator = T*;
		using const_iterator = const T*;

		template<typename A, typename B, typename... Args>
		Vec(A a, B b, Args... args) {
			build<0>(a, b, args...);
		}

		Vec(T t) {
			for(usize i = 0; i != N; i++) {
				_vec[i] = t;
			}
		}

		template<typename X>
		Vec(const Vec<N, X>& v) {
			for(usize i = 0; i != N; i++) {
				_vec[i] = v[i];
			}
		}

        Vec(detail::identity_t&&) : Vec() {
		}

		Vec() = default;
		Vec(const Vec &) = default;
		Vec& operator=(const Vec &) = default;

		T length2() const {
			T sum = 0;
			for(usize i = 0; i != N; i++) {
				sum += _vec[i] * _vec[i];
			}
			return sum;
		}

		auto length() const {
			return std::sqrt(length2());
		}

		T dot(const Vec& o) const {
			T sum = 0;
			for(usize i = 0; i != N; i++) {
				sum += _vec[i] * o._vec[i];
			}
			return sum;
		}

		Vec cross(const Vec& o) const {
			static_assert(std::is_signed<T>::value, "Vec<T>::cross makes no sense for T unsigned");
			Vec v;
			for(usize i = 0; i != N; i++) {
				v[i] = _vec[(i + 1) % N] * o._vec[(i + 2) % N] - _vec[(i + 2) % N] * o._vec[(i + 1) % N];
			}
			return v;
		}

		void normalize() {
			if(!is_zero()) {
				operator*=(1.0f / length());
			}
		}

		Vec normalized() const {
			Vec v(*this);
			v.normalize();
			return v;
		}

		Vec abs() const {
			static_assert(std::is_signed<T>::value, "Vec<T>::abs makes no sense for T unsigned");
			Vec v(*this);
			for(usize i = 0; i != N; i++) {
				v[i] = _vec[i] < 0 ? -_vec[i] : _vec[i];
			}
			return v;
		}

		T& x() {
			return _vec[0];
		}

		const T& x() const {
			return _vec[0];
		}

		T& y() {
			static_assert(N > 1, "Accessing out of bound member");
			return _vec[1];
		}

		const T& y() const {
			static_assert(N > 1, "Accessing out of bound member");
			return _vec[1];
		}

		T& z() {
			static_assert(N > 2, "Accessing out of bound member");
			return _vec[2];
		}

		const T& z() const {
			static_assert(N > 2, "Accessing out of bound member");
			return _vec[2];
		}

		T& w() {
			static_assert(N > 3, "Accessing out of bound member");
			return _vec[3];
		}

		const T& w() const {
			static_assert(N > 3, "Accessing out of bound member");
			return _vec[3];
		}

		auto sub(usize index) const {
			Vec<N - 1, T> v;
			for(usize i = 0; i != index; i++) {
				v[i] = _vec[i];
			}
			for(usize i = index; i < N - 1; i++) {
				v[i] = _vec[i + 1];
			}
			return v;
		}

		bool is_zero() const {
			for(usize i = 0; i != N; i++) {
				if(_vec[i]) {
					return false;
				}
			}
			return true;
		}

		const_iterator begin() const {
			return _vec;
		}

		const_iterator end() const {
			return _vec + N;
		}

		const_iterator cbegin() const {
			return _vec;
		}

		const_iterator cend() const {
			return _vec + N;
		}

		iterator begin() {
			return _vec;
		}

		iterator end() {
			return _vec + N;
		}

		T& operator[](usize i) {
			return _vec[i];
		}

		const T& operator[](usize i) const {
			return _vec[i];
		}

		bool operator!=(const Vec<N, T>& o) const {
			for(usize i = 0; i != N; i++) {
				if(o._vec[i] != _vec[i]) {
					return true;
				}
			}
			return false;
		}

		bool operator==(const Vec<N, T>& o) const {
			return !operator!=(o);
		}

		Vec operator-() const {
			Vec t;
			for(usize i = 0; i != N; i++) {
				t[i] = -_vec[i];
			}
			return t;
		}

		Vec& operator*=(const T& t) {
			for(usize i = 0; i != N; i++) {
				_vec[i] *= t;
			}
			return *this;
		}

		Vec& operator/=(const T& t) {
			for(usize i = 0; i != N; i++) {
				_vec[i] /= t;
			}
			return *this;
		}

		Vec& operator+=(const T& t) {
			for(usize i = 0; i != N; i++) {
				_vec[i] += t;
			}
			return *this;
		}

		Vec& operator-=(const T& t) {
			for(usize i = 0; i != N; i++) {
				_vec[i] -= t;
			}
			return *this;
		}



		Vec& operator*=(const Vec& v) {
			for(usize i = 0; i != N; i++) {
				_vec[i] *= v[i];
			}
			return *this;
		}

		Vec& operator/=(const Vec& v) {
			for(usize i = 0; i != N; i++) {
				_vec[i] /= v[i];
			}
			return *this;
		}

		Vec& operator+=(const Vec& v) {
			for(usize i = 0; i != N; i++) {
				_vec[i] += v[i];
			}
			return *this;
		}

		Vec& operator-=(const Vec& v) {
			for(usize i = 0; i != N; i++) {
				_vec[i] -= v[i];
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




namespace detail {

template<typename... Types>
struct VecCoerce {
};

template<typename T>
struct VecCoerce<T> {
	using type = T;
};

template<usize N, typename T>
struct VecCoerce<Vec<N, T>> {
	using type = T;
};

template<typename T, typename U>
struct VecCoerce<T, U> {
	using left = typename VecCoerce<T>::type;
	using right = typename VecCoerce<U>::type;

	using type = typename std::common_type<left, right>::type;
};

template<typename T, typename U, typename... Types>
struct VecCoerce<T, U, Types...> {
	using left = typename VecCoerce<T>::type;
	using right = typename VecCoerce<U, Types...>::type;

	using type = typename VecCoerce<left, right>::type;
};




template<typename... Types>
struct VecLen {
};

template<typename T>
struct VecLen<T> {
	static constexpr usize value = 1;
};

template<usize N, typename T>
struct VecLen<Vec<N, T>> {
	static constexpr usize value = N;
};

template<typename T, typename U>
struct VecLen<T, U> {
	static constexpr usize value = VecLen<T>::value + VecLen<U>::value;
};

template<typename T, typename U, typename... Types>
struct VecLen<T, U, Types...> {
	static constexpr usize value = VecLen<T>::value + VecLen<U, Types...>::value;
};


}

template<typename... Args>
auto vec(Args... args) {
	return Vec<detail::VecLen<Args...>::value, typename detail::VecCoerce<Args...>::type>(args...);
}


static_assert(std::is_same<decltype(vec(1, 2, 3)), Vec<3, int>>::value, "Invalid vec(...) return type");
static_assert(std::is_same<decltype(vec(1, 2.0)), Vec<2, double>>::value, "Invalid vec(...) return type");
static_assert(std::is_same<decltype(vec(1, 2.0f, 0)), Vec<3, float>>::value, "Invalid vec(...) return type");
static_assert(std::is_same<decltype(vec(1, vec(2.0, 3))), Vec<3, double>>::value, "Invalid vec(...) return type");
static_assert(std::is_same<decltype(vec(vec(1, 2))), Vec<2, int>>::value, "Invalid vec(...) return type");




namespace detail {

template<usize N, typename A, typename B>
struct V {
	using type = Vec<N, typename Coerce<A, B>::type>;
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
	return vec(l) / v;
}

template<usize N, typename T>
auto operator/(Vec<N, T> a, const Vec<N, T>& b) {
	a /= b;
	return a;
}

}
}


#endif // Y_MATH_VEC_H
