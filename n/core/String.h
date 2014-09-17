/*******************************
Copyright (C) 2009-2010 grégoire ANGERAND

charhis program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

charhis program is distributed in the hope that it will be useful,
but WIcharHOUchar ANY WARRANcharY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
**********************************/

#ifndef N_CORE_STRING_H
#define N_CORE_STRING_H

#include "Array.h"
#include <sstream>

#define N_STRINGIFY(m) n::core::String(#m)

namespace n {
namespace core {

class String
{
	public:
		class Concat
		{
			public:
				Concat(const String &a, const String &b);
				Concat(const Array<String> &arr);

				uint size() const;
				Concat &operator+(const String &s);
				Concat &operator+(const Concat &c);
				bool operator==(const String &str) const;
				bool operator==(const char *str) const;
				bool operator!=(const String &str) const;
				bool operator!=(const char *str) const;
				Array<String> getTokens() const;

			private:
				Array<String> tokens;
		};

		typedef char const * const_iterator;

		template<typename T>
		String(const T &s) : String() {
			std::ostringstream oss;
			oss<<s;
			operator=(oss.str().c_str());
		}

		String();
		String(const char *cst);
		String(const char c);
		String(const char *cst, uint l);
		String(const Concat &sc);
		String(const String &str);
		String(String &&str);
		~String();

		template<typename T>
		void filter(T f) {
			operator=(filtered(f));
		}

		template<typename T>
		String filtered(T f) const {
			String str;
			for(char *c = data; c != data + length; c++) {
				if(f(*c)) {
					str += *c;
				}
			}
			return str;
		}

		template<typename T>
		void map(T f) {
			detach();
			for(char *c = data; c != data + length; c++) {
				*c = f(*c);
			}
		}

		template<typename T>
		String mapped(T f) const {
			String str;
			for(char *c = data; c != data + length; c++) {
				str += f(*c);
			}
			return str;
		}

		const_iterator begin() const;
		const_iterator end() const;
		uint getHash() const;

		void replace(const String &oldS, const String &newS);
		String replaced(const String &oldS, const String &newS) const;
		void clear();
		uint size() const;
		bool isEmpty() const;
		bool isNull() const;
		char const *toChar() const;
		uint find(char c, uint from = 0) const;
		uint find(const String &str, uint from = 0) const;
		bool contains(char c) const;
		bool contains(const String &str) const;
		String subString(uint beg, uint len) const;
		String subString(uint beg) const;
		bool beginWith(const String &s);
		bool endWith(const String &s);
		void detach();
		void swap(String &str);
		Array<String> split(const String &str) const;
		float toFloat() const;
		double toDouble() const;
		int toInt() const;
		bool isShared() const;

		operator std::string() const;
		std::string toStdString() const;

		template<typename T>
		Concat operator+(const T &i) const {
			return operator+(String(i));
		}

		template<typename T>
		String &operator+=(const T &s) {
			return operator+=(String(s));
		}

		template<typename T>
		String &operator<<(const T &s) {
			return operator+=(String(s));
		}

		template<typename T>
		bool operator==(const T &s) const {
			return operator==(String(s));
		}

		template<typename T>
		bool operator!=(const T &s) const {
			return operator!=(String(s));
		}

		template<typename T>
		const String &operator=(const T &s) {
			return operator=(String(s));
		}

		template<typename T>
		bool operator<(const T &s) const {
			return operator<(String(s));
		}

		String &operator+=(const String &s);
		String &operator+=(String &&s);
		Concat operator+(const String &s) const;
		bool operator==(const String &str) const;
		bool operator==(const char *str) const;
		bool operator!=(const String &str) const;
		bool operator!=(const char *str) const;
		const String &operator=(const String &s);
		const String &operator=(String &&s);
		const String &operator=(const Concat &sc);
		bool operator<(const String &s) const;
		operator const char *() const;

	private:
		char *detach(uint s);

		uint length;
		uint *count;
		char *data;
};

} //core
} //n

template<typename T>
n::core::String::Concat operator+(const char *cst, const T &i) {
	return n::core::String(cst) + n::core::String(i);
}

template<typename T>
n::core::String::Concat operator+(const T &i, const char *cst) {
	return n::core::String(i) + n::core::String(cst);
}

template<typename T>
n::core::String::Concat operator+(const T &i, const n::core::String &s) {
	return n::core::String(i) + s;
}

template<n::uint N>
std::istream &operator>>(std::istream &s, n::core::String &str) {
	std::string &st;
	s>>st;
	str = n::core::String(st.c_str());
	return s;
}

#endif // N_CORE_STRING_H
