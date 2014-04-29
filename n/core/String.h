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

		char const *begin() const;
		char const *end() const;
		String &operator<<(const String &s);
		uint getHash() const;

		void replace(const String &oldS, const String &newS);
		String replaced(const String &oldS, const String &newS) const;
		void clear();
		uint size() const;
		bool isEmpty() const;
		char const *toChar() const;
		uint find(char c, uint from = 0) const;
		uint find(const String &str, uint from = 0) const;
		bool contains(char c) const;
		bool contains(String &str) const;
		String subString(uint beg, uint len) const;
		String subString(uint beg) const;
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

template<typename T>
String::Concat operator+(const char *cst, const T &i) {
	return String(cst) + String(i);
}

template<typename T>
String::Concat operator+(const T &i, const char *cst) {
	return String(i) + String(cst);
}

template<typename T>
String::Concat operator+(const T &i, const String &s) {
	return String(i) + s;
}


} //core
} //n

#endif // N_CORE_STRING_H
