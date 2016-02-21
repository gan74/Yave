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

#ifndef N_CORE_STRING1_H
#define N_CORE_STRING1_H

#include <sstream>
#include "Array.h"
#include "AsCollection.h"
#include <n/defines.h>
#include <n/concurrent/Atomic.h>

#define N_STRINGIFY(m) n::core::String(#m)

namespace n {
namespace core {

class String1
{
	public:
		typedef concurrent::auint CounterType;
		typedef char const * const_iterator;

		template<typename T, typename... Args>
		explicit String1(const T &s) : String1(build(s)) {
		}

		/*explicit */String1(const char *cst, uint l);

		String1();
		String1(const char *cst);
		String1(const String1 &str);
		String1(String1 &&str);
		~String1();

		const_iterator begin() const;
		const_iterator end() const;
		uint64 getHash() const;

		void replace(uint beg, uint len, const String1 &newS);
		void replace(const String1 &oldS, const String1 &newS);
		String1 replaced(uint beg, uint len, const String1 &newS) const;
		String1 replaced(const String1 &oldS, const String1 &newS) const;
		void clear();
		uint size() const;
		bool isEmpty() const;
		bool isNull() const;
		uint find(char c, uint from = 0) const;
		uint find(const String1 &str, uint from = 0) const;
		bool contains(char c) const;
		bool contains(const String1 &str) const;
		String1 subString(uint beg, uint len) const;
		String1 subString(uint beg) const;
		bool beginsWith(const String1 &s) const;
		bool endsWith(const String1 &s) const;
		void detach();
		void swap(String1 &str);
		Array<String1> split(const String1 &str, bool empties = false) const;
		String1 toLower() const;
		String1 toUpper() const;
		String1 trimmed() const;
		bool isShared() const;
		bool isUnique() const;
		bool isSharedSubset() const;


		template<typename T>
		String1 &operator=(const T &e) {
			return operator=(String1(e));
		}

		template<typename T>
		String1 operator+(const T &i) const {
			return operator+(String1(i));
		}

		template<typename T>
		String1 &operator+=(const T &i) {
			return operator+=(String1(i));
		}

		String1 &operator+=(const String1 &s);
		String1 operator+(const String1 &s) const;
		bool operator==(const String1 &str) const;
		bool operator==(const char *str) const;
		bool operator!=(const String1 &str) const;
		bool operator!=(const char *str) const;
		String1 &operator=(const String1 &s);
		String1 &operator=(String1 &&s);
		bool operator<(const String1 &s) const;
		bool operator>(const String1 &s) const;

		const char &operator[](uint i) const {
			return data[i];
		}

		char &operator[](uint i) {
			detach();
			return data[i];
		}

		const char *toChar() const;
		explicit operator const char *() const;

		std::string toStdString() const;

		template<typename T>
		explicit operator T() const {
			return to<T>();
		}

		template<typename T, typename F = Nothing>
		T to(F f = F()) const {
			if(std::is_same<T, std::string>::value) {
				std::string s = toStdString();
				void *v = &s;
				return *reinterpret_cast<T *>(v);
			}
			std::istringstream str;
			str.rdbuf()->pubsetbuf(data, length);
			T t;
			str>>t;
			if(str.fail() || !str.eof()) {
				f();
				return T();
			}
			return t;
		}

		template<typename T, typename F = Nothing>
		T get(F f = F()) const {
			if(std::is_same<T, std::string>::value) {
				std::string s = toStdString();
				void *v = &s;
				return *reinterpret_cast<T *>(v);
			}
			std::istringstream str;
			str.rdbuf()->pubsetbuf(data, length);
			T t;
			str>>t;
			if(str.fail()) {
				f();
				return T();
			}
			return t;
		}

		operator std::string() const {
			return toStdString();
		}

		explicit operator bool() const {
			if((int)(*this)) {
				return true;
			}
			std::istringstream str;
			str.rdbuf()->pubsetbuf(data, length);
			bool t = false;
			str>>std::boolalpha>>t;
			return t;
		}

		template<typename T>
		void filter(T f) {
			operator=(filtered(f));
		}

		template<typename T>
		String1 filtered(T f) const {
			String1 str;
			for(char *c = data; c != data + length; c++) {
				if(f(*c)) {
					str += String1(*c);
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
		String1 mapped(T f) const {
			String1 str(0, size());
			for(uint i = 0; i != size(); i++) {
				str.data[i] = f(data[i]);
			}
			return str;
		}

	private:
		String1(const String1 &str, uint beg, uint len);

		struct StringConverter;

		template<typename T>
		N_FORCE_INLINE String1 build(const T &t) const {
			return convertDispatch(t, BoolToType<TypeConversion<T, StringConverter>::canBuild>());
		}

		template<typename T>
		N_FORCE_INLINE String1 convertDispatch(const T &t, FalseType) const {
			return buildDispatch(t, BoolToType<Collection<T>::isCollection>(), BoolToType<std::is_same<typename Collection<T>::Element, String1>::value>());
		}

		N_FORCE_INLINE const String1 &convertDispatch(const String1 &t, TrueType) const {
			return t;
		}

		template<typename T, typename B>
		N_FORCE_INLINE String1 buildDispatch(const T &t, FalseType, B) const {
			std::ostringstream oss;
			oss<<t;
			return oss.str().c_str();
		}

		template<typename B>
		N_FORCE_INLINE String1 buildDispatch(const bool &t, FalseType, B) const {
			std::ostringstream oss;
			oss<<std::boolalpha<<t;
			return oss.str().c_str();
		}

		template<typename T>
		N_FORCE_INLINE String1 buildDispatch(const T &t, TrueType, FalseType) const {
			String1 self;
			for(auto c : t) {
				self += String1(c);
			}
			return self;
		}

		template<typename T>
		N_FORCE_INLINE String1 buildDispatch(const T &t, TrueType, TrueType) const {
			uint l = 0;
			for(const String1 &c : t) {
				l += c.size();
			}
			String1 str(0, l);
			char *ptr = str.data;
			for(const String1 &c : t) {
				memcpy(ptr, c.data, c.length * sizeof(char));
				ptr += c.length;
			}
			return str;
		}

		template<typename B>
		N_FORCE_INLINE const String1 &buildDispatch(const String1 &t, TrueType, B) const {
			return t;
		}

		template<typename B>
		N_FORCE_INLINE const String1 &buildDispatch(const String1 &t, FalseType, B) const {
			return t;
		}

		void detach(uint s) const;

		mutable uint length;
		mutable CounterType *count;
		mutable char *data;
};

struct String1::StringConverter
{
	StringConverter(const String1 &) {
	}
};


}
}


N_GEN_CLASS_OP(n::core::String1, +)
N_GEN_CLASS_OP(n::core::String1, <)
N_GEN_CLASS_OP(n::core::String1, >)
N_GEN_CLASS_OP(n::core::String1, ==)
N_GEN_CLASS_OP(n::core::String1, !=)


std::istream &operator>>(std::istream &s, n::core::String1 &str);
std::ostream &operator<<(std::ostream &s, const n::core::String1 &str);

#endif // N_CORE_STRING1_H
