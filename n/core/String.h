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

#ifndef N_CORE_STRING_H
#define N_CORE_STRING_H

#include <sstream>
#include "Array.h"
#include "AsCollection.h"
#include <n/defines.h>
#include <n/concurent/Atomic.h>

#define N_STRINGIFY(m) n::core::String(#m)

namespace n {
namespace core {

class String
{
	public:
		typedef concurent::auint CounterType;
		typedef char const * const_iterator;

		static_assert(TypeInfo<CounterType>::isPod, "String::CounterType need to be POD");

		template<typename T, typename... Args>
		explicit String(const T &s) : String(build(s)) {
		}

		/*explicit */String(const char *cst, uint l);

		String();
		String(const char *cst);
		String(const String &str);
		String(String &&str);
		~String();

		const_iterator begin() const;
		const_iterator end() const;
		uint getHash() const;

		void replace(uint beg, uint len, const String &newS);
		void replace(const String &oldS, const String &newS);
		String replaced(uint beg, uint len, const String &newS) const;
		String replaced(const String &oldS, const String &newS) const;
		void clear();
		uint size() const;
		bool isEmpty() const;
		bool isNull() const;
		uint find(char c, uint from = 0) const;
		uint find(const String &str, uint from = 0) const;
		bool contains(char c) const;
		bool contains(const String &str) const;
		String subString(uint beg, uint len) const;
		String subString(uint beg) const;
		bool beginWith(const String &s) const;
		bool endWith(const String &s) const;
		void detach();
		void swap(String &str);
		Array<String> split(const String &str, bool empties = false) const;
		String toLower() const;
		String toUpper() const;
		bool isShared() const;
		bool isUnique() const;
		bool isSharedSubset() const;

		template<typename T>
		String &operator=(const T &e) {
			return operator=(String(e));
		}

		String &operator+=(const String &s);
		String operator+(const String &s) const;
		bool operator==(const String &str) const;
		bool operator==(const char *str) const;
		bool operator!=(const String &str) const;
		bool operator!=(const char *str) const;
		String &operator=(const String &s);
		String &operator=(String &&s);
		bool operator<(const String &s) const;

		const char &operator[](uint i) const {
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
			if(str.fail()) {
				f();
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
		String filtered(T f) const {
			String str;
			for(char *c = data; c != data + length; c++) {
				if(f(*c)) {
					str += String(*c);
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
			String str(0, size());
			for(uint i = 0; i != size(); i++) {
				str.data[i] = f(data[i]);
			}
			return str;
		}

	private:
		String(const String &str, uint beg, uint len);

		struct StringConverter;

		template<typename T>
		N_FORCE_INLINE String build(const T &t) const {
			return convertDispatch(t, BoolToType<TypeConversion<T, StringConverter>::exists>());
		}

		template<typename T>
		N_FORCE_INLINE String convertDispatch(const T &t, FalseType) const {
			return buildDispatch(t, BoolToType<Collection<T>::isCollection>(), BoolToType<std::is_same<typename Collection<T>::ElementType, String>::value>());
		}

		N_FORCE_INLINE const String &convertDispatch(const String &t, TrueType) const {
			return t;
		}

		template<typename T, typename B>
		N_FORCE_INLINE String buildDispatch(const T &t, FalseType, B) const {
			std::ostringstream oss;
			oss<<t;
			return oss.str().c_str();
		}

		template<typename B>
		N_FORCE_INLINE String buildDispatch(const bool &t, FalseType, B) const {
			std::ostringstream oss;
			oss<<std::boolalpha<<t;
			return oss.str().c_str();
		}

		template<typename T>
		N_FORCE_INLINE String buildDispatch(const T &t, TrueType, FalseType) const {
			String self;
			for(auto c : t) {
				self += String(c);
			}
			return self;
		}

		template<typename T>
		N_FORCE_INLINE String buildDispatch(const T &t, TrueType, TrueType) const {
			uint l = 0;
			for(const String &c : t) {
				l += c.size();
			}
			String str(0, l);
			char *ptr = str.data;
			for(const String &c : t) {
				memcpy(ptr, c.data, c.length * sizeof(char));
				ptr += c.length;
			}
			return str;
		}

		template<typename B>
		N_FORCE_INLINE const String &buildDispatch(const String &t, TrueType, B) const {
			return t;
		}

		template<typename B>
		N_FORCE_INLINE const String &buildDispatch(const String &t, FalseType, B) const {
			return t;
		}

		void detach(uint s) const;

		mutable uint length;
		mutable CounterType *count;
		mutable char *data;
};

struct String::StringConverter : public String
{
	template<typename T>
	explicit StringConverter(const T &t) : String(t) {
	}
};


}
}

template<typename T>
n::core::String operator+(const T &i, const n::core::String &s) {
	return n::core::String(i) + s;
}

template<typename T>
n::core::String operator+(const n::core::String &s, const T &i) {
	return s + n::core::String(i);
}

std::istream &operator>>(std::istream &s, n::core::String &str);
std::ostream &operator<<(std::ostream &s, const n::core::String &str);

#endif // N_CORE_STRING_H
