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

#ifndef VEC_H
#define VEC_H

#include <n/core/String.h>

namespace n {
namespace math {

template<typename T, uint N>
class Vec
{
	template<uint P, typename... Args>
	void build(T t, Args... args) {
		vec[P] = t;
		build<P + 1>(args...);
	}

	template<uint P, uint Q, typename U, typename... Args>
	void build(const Vec<U, Q> &t, Args... args) {
		for(uint i = 0; i != Q; i++) {
			vec[P + i] = t[i];
		}
		build<P + Q>(args...);
	}

	template<uint P>
	void build() {
		static_assert(P == N, "Wrong number of arguments");
	}


	public:
		typedef T * iterator;
		typedef const T *const_iterator;

		template<typename... Args>
		Vec(T x, Args... args) {
			build<0>(x, args...);
		}

		Vec(T t = T(0)) {
			for(uint i = 0; i != N; i++) {
				vec[i] = t;
			}
		}

		template<typename X>
		Vec(const Vec<X, N> &v) {
			for(uint i = 0; i != N; i++) {
				vec[i] = v[i];
			}
		}

		Vec(Vec<T, N - 1> v, T t = (T)0) {
			for(uint i = 0; i != N - 1; i++) {
				vec[i] = v[i];
			}
			vec[N - 1] = t;
		}


		T length2() const {
			T sum = 0;
			for(uint i = 0; i != N; i++) {
				sum += vec[i] * vec[i];
			}
			return sum;
		}

		T length() const {
			return sqrt(length2());
		}

		T dot(const Vec<T, N> &o) const {
			T sum = 0;
			for(uint i = 0; i != N; i++) {
				sum += vec[i] * o.vec[i];
			}
			return sum;
		}

		Vec<T, N> cross(const Vec<T, N> &o) const {
			Vec<T, N> v;
			for(uint i = 0; i != N; i++) {
				v[i] = vec[(i + 1) % N] * o.vec[(i + 2) % N] - vec[(i + 2) % N] * o.vec[(i + 1) % N];
			}
			return v;
		}

		void normalize() {
			if(!isNull()) {
				this->operator*=(1.0f / length());
			}
		}

		void saturate() {
			this->operator*=(1.0f / max());
		}

		Vec<T, N> normalized() const {
			if(isNull()) {
				return Vec<T, N>();
			}
			Vec<T, N> v(vec);
			return v * (1.0f / length());
		}

		Vec<T, N> saturated() const {
			Vec<T, N> v(vec);
			return v * (1.0f / max());
		}

		Vec<T, N> abs() const {
			Vec<T, N> v(vec);
			for(uint i = 0; i != N; i++) {
				v[i] = vec[i] < T(0) ? -vec[i] : vec[i];
			}
			return v;
		}

		Vec<T, N> max(const Vec<T, N> &v) const {
			Vec<T, N> e;
			for(uint i = 0; i != N; i++) {
				e[i] = std::max(v[i], vec[i]);
			}
			return e;
		}

		T max() const {
			T t = vec[0];
			for(uint i = 1; i < N; i++) {
				t = std::max(t, vec[i]);
			}
			return t;
		}

		Vec<T, N> min(const Vec<T, N> &v) const {
			Vec<T, N> e;
			for(uint i = 0; i != N; i++) {
				e[i] = std::min(v[i], vec[i]);
			}
			return e;
		}

		T min() const {
			T t = vec[0];
			for(uint i = 1; i < N; i++) {
				t = std::min(t, vec[i]);
			}
			return t;
		}

		T mul() const {
			T v = 1;
			for(uint i = 0; i != N; i++) {
				v *= vec[i];
			}
			return v;
		}

		T sum() const {
			T v = 0;
			for(uint i = 0; i != N; i++) {
				v += vec[i];
			}
			return v;
		}

		T &x() {
			return vec[0];
		}

		const T &x() const {
			return vec[0];
		}

		T &y() {
			return vec[1];
		}

		const T &y() const {
			return vec[1];
		}

		T &z() {
			return vec[2];
		}

		const T &z() const {
			return vec[2];
		}

		T &w() {
			return vec[3];
		}

		const T &w() const {
			return vec[3];
		}

		bool isNull() const {
			for(uint i = 0; i != N; i++) {
				if(vec[i]) {
					return false;
				}
			}
			return true;
		}

		Vec<T, N - 1> sub(uint w) const {
			Vec<T, N - 1> s;
			uint j = 0;
			for(uint i = 0; i != N; i++) {
				if(i != w) {
					s[j++] = vec[i];
				}
			}
			return s;
		}

		const_iterator begin() const {
			return vec;
		}

		const_iterator end() const {
			return vec + N;
		}

		iterator begin() {
			return vec;
		}

		iterator end() {
			return vec + N;
		}

		T &operator[](uint i) {
			return vec[i];
		}

		const T &operator[](uint i) const {
			return vec[i];
		}

		bool operator!=(const Vec<T, N> &o) const {
			for(uint i = 0; i != N; i++) {
				if(o.vec[i] != vec[i]) {
					return true;
				}
			}
			return false;
		}

		bool operator==(const Vec<T, N> &o) const {
			return !operator!=(o);
		}

		Vec<T, N> operator+(const Vec<T, N> &v) const {
			T t[N];
			for(uint i = 0; i != N; i++) {
				t[i] = vec[i] + v.vec[i];
			}
			return Vec<T, N>(t);
		}

		Vec<T, N> operator-(const Vec<T, N> &v) const {
			T t[N];
			for(uint i = 0; i != N; i++) {
				t[i] = vec[i] - v.vec[i];
			}
			return Vec<T, N>(t);
		}

		Vec<T, N> operator*(const Vec<T, N> &v) const {
			T t[N];
			for(uint i = 0; i != N; i++) {
				t[i] = vec[i] * v.vec[i];
			}
			return Vec<T, N>(t);
		}

		Vec<T, N> operator/(const Vec<T, N> &v) const {
			T t[N];
			for(uint i = 0; i != N; i++) {
				t[i] = vec[i] / v.vec[i];
			}
			return Vec<T, N>(t);
		}

		Vec<T, N> operator-() const {
			T t[N];
			for(uint i = 0; i != N; i++) {
				t[i] = -vec[i];
			}
			return Vec<T, N>(t);
		}

		Vec<T, N> &operator+=(const Vec<T, N> &v) {
			for(uint i = 0; i != N; i++) {
				vec[i] += v.vec[i];
			}
			return *this;
		}

		Vec<T, N> &operator-=(const Vec<T, N> &v) {
			for(uint i = 0; i != N; i++) {
				vec[i] -= v.vec[i];
			}
			return *this;
		}

		Vec<T, N> operator*(const T &t) const {
			T v[N];
			for(uint i = 0; i != N; i++) {
				v[i] = vec[i] * t;
			}
			return Vec<T, N>(v);
		}

		Vec<T, N> operator/(const T &t) const {
			T v[N];
			for(uint i = 0; i != N; i++) {
				v[i] = vec[i] / t;
			}
			return Vec<T, N>(v);
		}

		Vec<T, N> &operator*=(const T &t) {
			for(uint i = 0; i != N; i++) {
				vec[i] *= t;
			}
			return *this;
		}

		Vec<T, N> &operator/=(const T &t) {
			for(uint i = 0; i != N; i++) {
				vec[i] /= t;
			}
			return *this;
		}

		Vec<T, N> operator^(const Vec<T, N> &o) const {
			return cross(o);
		}

		bool operator<(const Vec<T, N> &v) const {
			for(uint i = 0; i != N; i++) {
				if(vec[i] < v[i]) {
					return true;
				} else if(vec[i] > v[i]) {
					return false;
				}
			}
			return false;
		}

		bool operator>(const Vec<T, N> &v) const {
			for(uint i = 0; i != N; i++) {
				if(vec[i] > v[i]) {
					return true;
				} else if(vec[i] < v[i]) {
					return false;
				}
			}
			return false;
		}

		operator core::String() const {
			return "(" + (core::AsCollection<Vec<T, N>>(*this)).make(core::String(", ")) + ")";
		}

	private:
		T vec[N];

};


typedef Vec<float, 2> Vec2;
typedef Vec<float, 3> Vec3;
typedef Vec<float, 4> Vec4;

typedef Vec<double, 2> Vec2d;
typedef Vec<double, 3> Vec3d;
typedef Vec<double, 4> Vec4d;

typedef Vec<int, 2> Vec2i;
typedef Vec<int, 3> Vec3i;
typedef Vec<int, 4> Vec4i;

typedef Vec<uint, 2> Vec2ui;
typedef	Vec<uint, 3> Vec3ui;
typedef Vec<uint, 4> Vec4ui;

}
}

#endif // VEC_H
