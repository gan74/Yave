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

#ifndef N_MATH_VEC_H
#define N_MATH_VEC_H

namespace n {
namespace math {

template<uint N, typename T = float>
class Vec
{
	template<uint P, typename... Args>
	void build(T t, Args... args) {
		vec[P] = t;
		build<P + 1>(args...);
	}

	template<uint P, uint Q, typename... Args>
	void build(const Vec<Q, T> &t, Args... args) {
		for(uint i = 0; i != Q; i++) {
			vec[P + i] = t[i];
		}
		build<P + Q>(args...);
	}

	template<uint P>
	void build() {
		static_assert(P == N, "Wrong number of arguments");
	}


	static_assert(N != 0, "Can not create a Vec of dimension 0");


	public:
		using iterator = T *;
		using const_iterator = const T *;

		template<typename U, typename V, typename... Args>
		Vec(U x, V y, Args... args) {
			build<0>(x, y, args...);
		}

		template<typename U, typename C = typename std::enable_if<TypeConversion<U, T>::exists, U>::type>
		Vec(U t) {
			for(uint i = 0; i != N; i++) {
				vec[i] = t;
			}
		}

		Vec() {
			for(uint i = 0; i != N; i++) {
				vec[i] = T(0);
			}
		}

		template<typename X>
		Vec(const Vec<N, X> &v) {
			for(uint i = 0; i != N; i++) {
				vec[i] = v[i];
			}
		}

		T length2() const {
			T sum = 0;
			for(uint i = 0; i != N; i++) {
				sum += vec[i] * vec[i];
			}
			return sum;
		}

		T length() const {
			return std::sqrt(length2());
		}

		T dot(const Vec<N, T> &o) const {
			T sum = 0;
			for(uint i = 0; i != N; i++) {
				sum += vec[i] * o.vec[i];
			}
			return sum;
		}

		Vec<N, T> cross(const Vec<N, T> &o) const {
			Vec<N, T> v;
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

		Vec<N, T> normalized() const {
			if(isNull()) {
				return Vec<N, T>();
			}
			Vec<N, T> v(*this);
			return v * (1.0f / length());
		}

		Vec<N, T> saturated() const {
			Vec<N, T> v(*this);
			return v * (1.0f / max());
		}

		Vec<N, T> abs() const {
			Vec<N, T> v(*this);
			for(uint i = 0; i != N; i++) {
				v[i] = vec[i] < T(0) ? -vec[i] : vec[i];
			}
			return v;
		}

		Vec<N, T> max(const Vec<N, T> &v) const {
			Vec<N, T> e;
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

		Vec<N, T> min(const Vec<N, T> &v) const {
			Vec<N, T> e;
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
			static_assert(N > 1, "Accessing out of bound member");
			return vec[1];
		}

		const T &y() const {
			static_assert(N > 1, "Accessing out of bound member");
			return vec[1];
		}

		T &z() {
			static_assert(N > 2, "Accessing out of bound member");
			return vec[2];
		}

		const T &z() const {
			static_assert(N > 2, "Accessing out of bound member");
			return vec[2];
		}

		T &w() {
			static_assert(N > 3, "Accessing out of bound member");
			return vec[3];
		}

		const T &w() const {
			static_assert(N > 3, "Accessing out of bound member");
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

		Vec<N - 1, T> sub(uint w) const {
			Vec<N - 1, T> s;
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

		Vec<N, T> flipped() const {
			Vec<N, T> v;
			for(uint i = 0; i != N; i++) {
				v[N - i - 1] = vec[i];
			}
			return v;
		}

		void flip()	{
			operator=(flipped());
		}

		T &operator[](uint i) {
			return vec[i];
		}

		const T &operator[](uint i) const {
			return vec[i];
		}

		bool operator!=(const Vec<N, T> &o) const {
			for(uint i = 0; i != N; i++) {
				if(o.vec[i] != vec[i]) {
					return true;
				}
			}
			return false;
		}

		bool operator==(const Vec<N, T> &o) const {
			return !operator!=(o);
		}

		Vec<N, T> operator+(const Vec<N, T> &v) const {
			Vec<N, T> t;
			for(uint i = 0; i != N; i++) {
				t[i] = vec[i] + v.vec[i];
			}
			return t;
		}

		Vec<N, T> operator-(const Vec<N, T> &v) const {
			Vec<N, T> t;
			for(uint i = 0; i != N; i++) {
				t[i] = vec[i] - v.vec[i];
			}
			return t;
		}

		Vec<N, T> operator*(const Vec<N, T> &v) const {
			Vec<N, T> t;
			for(uint i = 0; i != N; i++) {
				t[i] = vec[i] * v.vec[i];
			}
			return t;
		}

		Vec<N, T> operator/(const Vec<N, T> &v) const {
			Vec<N, T> t;
			for(uint i = 0; i != N; i++) {
				t[i] = vec[i] / v.vec[i];
			}
			return t;
		}

		Vec<N, T> operator-() const {
			Vec<N, T> t;
			for(uint i = 0; i != N; i++) {
				t[i] = -vec[i];
			}
			return t;
		}

		Vec<N, T> operator*(const T &t) const {
			Vec<N, T> v;
			for(uint i = 0; i != N; i++) {
				v[i] = vec[i] * t;
			}
			return v;
		}

		Vec<N, T> operator/(const T &t) const {
			Vec<N, T> v;
			for(uint i = 0; i != N; i++) {
				v[i] = vec[i] / t;
			}
			return v;
		}

		template<typename U>
		Vec<N, T> &operator*=(const U &t) {
			this->operator=(*this * t);
			return *this;
		}

		template<typename U>
		Vec<N, T> &operator/=(const U &t) {
			this->operator=(*this / t);
			return *this;
		}

		template<typename U>
		Vec<N, T> &operator+=(const U &t) {
			this->operator=(*this + t);
			return *this;
		}

		template<typename U>
		Vec<N, T> &operator-=(const U &t) {
			this->operator=(*this - t);
			return *this;
		}

		Vec<N, T> operator^(const Vec<N, T> &o) const {
			return cross(o);
		}

		bool operator<(const Vec<N, T> &v) const {
			for(uint i = 0; i != N; i++) {
				if(vec[i] < v[i]) {
					return true;
				} else if(vec[i] > v[i]) {
					return false;
				}
			}
			return false;
		}

		bool operator>(const Vec<N, T> &v) const {
			for(uint i = 0; i != N; i++) {
				if(vec[i] > v[i]) {
					return true;
				} else if(vec[i] < v[i]) {
					return false;
				}
			}
			return false;
		}

	private:
		template<uint M, typename U>
		friend class Vec;

		T vec[N];

};


using Vec2 = Vec<2>;
using Vec3 = Vec<3>;
using Vec4 = Vec<4>;

using Vec2d = Vec<2, double>;
using Vec3d = Vec<3, double>;
using Vec4d = Vec<4, double>;

using Vec2i = Vec<2, int>;
using Vec3i = Vec<3, int>;
using Vec4i = Vec<4, int>;

using Vec2ui = Vec<2, uint>;
using Vec3ui = Vec<3, uint>;
using Vec4ui = Vec<4, uint>;


}
}

#define N_VEC_OP(op) \
template<n::uint N, typename T> \
n::math::Vec<N, T> operator op(const T &i, const n::math::Vec<N, T> &v) { return n::math::Vec<N, T>(i) op v; }

N_VEC_OP(+)
N_VEC_OP(*)
N_VEC_OP(-)
N_VEC_OP(/)


#undef N_VEC_OP


#endif // N_MATH_VEC_H
