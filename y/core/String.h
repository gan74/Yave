/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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
#ifndef Y_CORE_STRING_H
#define Y_CORE_STRING_H

#include <y/utils.h>
#include "Vector.h"
#include <sstream>


namespace y {
namespace core {

class String {
	struct LongLenType
	{
		usize len : 8 * sizeof(usize) - 1;
		usize is_long : 1;

		LongLenType(usize l = 0) : len(l), is_long(1) {
		}

		operator usize() const {
			return len;
		}
	};

	struct ShortLenType
	{
		u8 len : 7;
		u8 is_long : 1;

		ShortLenType(usize l = 0) : len(MaxShortSize - l), is_long(0) {
		}

		operator usize() const {
			return MaxShortSize - len;
		}
	};

	struct LongData
	{
		char *data;
		usize capacity;
		LongLenType length;

		LongData();
		LongData(const LongData &l);
		LongData(LongData &&l);
		LongData(const char *str, usize cap, usize len);
		LongData(const char *str, usize len);

		LongData &operator=(const LongData &) = delete;
	};

	struct ShortData
	{
		char data[sizeof(LongData) - 1];
		ShortLenType length;

		ShortData();
		ShortData(const ShortData &s);
		ShortData(const char *str, usize len);

		ShortData &operator=(const ShortData &) = delete;

	};

	static_assert(sizeof(ShortData) == sizeof(LongData), "String::LongData should be the same length as String::ShortData");

	public:
		static constexpr usize MaxShortSize = sizeof(ShortData::data);

		using iterator = char *;
		using const_iterator = const char *;

		String();
		String(const String &str);
		String(String &&str);

		String(const char *str); // NOT explicit
		String(const char *str, usize len);
		String(const char *beg, const char *end);

		~String();

		template<typename T>
		static String from(const T &t);
		static String from_owned(char *owned);

		usize size() const;
		usize capacity() const;
		bool is_empty() const;
		bool is_long() const;

		void clear();

		char *data();
		const char *data() const;

		iterator find(const String &str);
		const_iterator find(const String &str) const;

		operator const char *() const;
		operator char *();

		void swap(String &str);

		String &operator=(const String &str);
		String &operator=(String &&str);

		String &operator+=(const String &str);

		char &operator[](usize i);
		char operator[](usize i) const;

		Vector<u32> to_unicode() const;


		iterator begin() {
			return data();
		}

		iterator end() {
			return data() + size();
		}

		const_iterator begin() const {
			return data();
		}

		const_iterator end() const {
			return data() + size();
		}

		const_iterator cbegin() const {
			return data();
		}

		const_iterator cend() const {
			return data() + size();
		}

	private:
		union
		{
			LongData l;
			ShortData s;
		};

		const String *const_this() {
			return this;
		}

		static char *alloc_long(usize capacity);
		static usize compute_capacity(usize len);
		static void free_long(LongData &d);

};



namespace detail {
template<typename T>
inline String str(const T &t, std::false_type) {
	return String::from(t);
}

template<typename T>
inline String str(const T &t, std::true_type) {
	return String(t);
}
}

template<typename... Args>
inline String str(Args... args) {
	return String(args...);
}

template<typename T>
inline String str(const T &t) {
	return detail::str(t, bool_type<std::is_convertible<T, String>::value>());
}

inline String str_from_owned(char *owned) {
	return String::from_owned(owned);
}

template<typename T>
inline String str_from(const T &t) {
	return String::from(t);
}






template<typename T>
String String::from(const T &t) {
	std::ostringstream oss;
	oss << t;
	return oss.str().c_str();
}

}
}

#endif // Y_CORE_STRING_H
