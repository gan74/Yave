#ifndef N_CORE_BIGUINT_H
#define N_CORE_BIGUINT_H

#include <n/Types.h>

namespace n {
namespace core {

template<uint N>
class BigUint
{
	public:
		BigUint(uint32 l = 0) : v{l} {
		}

		template<uint M>
		BigUint(const BigUint<M> &n) : v{0} {
			for(uint i = 0; i < N && i < M; i++) {
				v[i] = n.v[i];
			}
		}

		BigUint(char const *s) : v{0} {
			for(char const *p = s; *p; ++p) {
				mul(10); add(*p - '0');
			}
		}

		BigUint<N> operator+(const BigUint<N> &n) const {
			BigUint<N> r(*this);
			for(uint i = 0; i < N; i++) {
				r.add(n.v[i], i);
			}
			return r;
		}

		BigUint<N> operator+=(const BigUint<N> &n) {
			return *this = *this + n;
		}

		BigUint<N> operator-(const BigUint<N> &n) const {
			BigUint<N> r(*this);
			for(uint i = 0; i < N; i++) {
				r.sub(n.v[i], i);
			}
			return r;
		}

		BigUint<N> operator-=(const BigUint<N> &n) {
			return *this = *this - n;
		}

		BigUint<N> operator*(const BigUint<N> &n) const {
			BigUint<N> r;
			BigUint<N> t;
			for(uint i = 0; i < N; i++) {
				(t = *this).mul(n.v[i], i);
				r += t;
			}
			return r;
		}

		BigUint<N> operator*=(const BigUint<N> &n) {
			return *this = *this *n;
		}

		void invert() {
			for(uint i = 0; i < N; i++) {
				v[i] = ~v[i];
			}
		}

		BigUint<N> operator~() const {
			BigUint<N> r(*this);
			r.invert();
			return r;
		}

		BigUint<N> &operator&=(const BigUint<N> &n) {
			for(uint i = 0; i < N; i++) {
				v[i] &= n.v[i];
			}
			return *this;
		}

		BigUint<N> operator&(const BigUint<N> &n) const {
			return BigUint<N>(*this) &= n;
		}

		BigUint<N> &operator|=(const BigUint<N> &n) {
			for(uint i = 0; i < N; i++) {
				v[i] |= n.v[i];
			}
			return *this;
		}

		BigUint<N> operator|(const BigUint<N> &n) const {
			return BigUint<N>(*this) |= n;
		}

		BigUint<N> &operator^=(const BigUint<N> &n) {
			for(uint i = 0; i < N; i++) {
				v[i] ^= n.v[i];
			}
			return *this;
		}

		BigUint<N> operator^(const BigUint<N> &n) const {
			return BigUint<N>(*this) ^= n;
		}

		bool isZero() const {
			for(uint i = N - 1; i < N; --i) {
				if(v[i]) {
					return false;
				}
			}
			return true;
		}

		int compare(const BigUint<N> &o) const {
			for(uint i = N - 1; i < N; --i) {
				if (v[i] < o.v[i]) {
					return -1;
				} else if (v[i] > o.v[i]) {
					return 1;
				}
			}
			return 0;
		}

		bool operator<(const BigUint<N> &o) const {
			return compare(o) < 0;
		}

		bool operator>(const BigUint<N> &o) const {
			return compare(o) > 0;
		}

		bool operator<=(const BigUint<N> &o) const {
			return compare(o) <= 0;
		}

		bool operator>=(const BigUint<N> &o) const {
			return compare(o) >= 0;
		}

		bool operator==(const BigUint<N> &o) const {
			return compare(o) == 0;
		}

		bool operator!=(const BigUint<N> &o) const {
			return compare(o) != 0;
		}

		void add(uint32 n, uint o = 0) {
			uint c = (v[o] += n) < n;
			for(uint i = o + 1; c && i < N; i++) {
				c = !++v[i];
			}
		}

		void sub(uint32 n, uint o = 0) {
			uint c = (v[o] -= n) > n;
			for(uint i = o + 1; c && i < N; i++) {
				c = !v[i]--;
			}
		}

		void mul(uint32 n, uint o = 0) {
			uint64 x, c = 0;
			BigUint<N> t(*this);
			for(uint i = 0; i + o < N; i++) {
				x = t.v[i]; x *= n; x += c;
				v[i + o] = x;
				c = x >> 32;
			}
			for(uint i = 0; i < o; i++) {
				v[i] = 0;
			}
		}

		void div(uint32 n, uint o = 0) {
			uint64 x = 0;
			BigUint<N> t(*this);
			for(uint i = N - 1; i - o < N; --i) {
				x = x << 32 | t.v[i];
				v[i - o] = x / n;
				x %= n;
			}
		}

		uint32 v[N];

};

} //core
} //n

template<n::uint N>
std::ostream &operator<<(std::ostream &st, n::core::BigUint<N> t) {
	char b[N *32];
	n::uint32 u;
	char s[N *32];
	char *i = s + N *32 - 1;
	char *j = b;
	for (*i-- = 0;; --i) {
		u = t.v[0];
		t.div(10);
		*i = u - t.v[0] *10 + '0';
		if(t.isZero()) {
			while((*j++ = *i++));
			return j - b;
		}
	}
	return st << b;
}

template<n::uint N>
std::istream &operator>>(std::istream &s, n::core::BigUint<N> &n) {
	std::string b;
	int c;
	while(c = s.peek(), c == ' ' || c == '\n' || c == '\r' || c == '\t') {
		s.ignore();
	}
	while(c = s.peek(), c >= '0' && c <= '9') {
		b.push_back(s.get());
	}
	n = n::core::BigUint<N>(b.c_str());
	return s;
}


#endif // N_CORE_BIGUINT_H
