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
#ifndef N_CORE_STRING2_H
#define N_CORE_STRING2_H

#include <n/utils.h>

namespace n {
namespace core {

class String2
{
	struct LongLenType
	{
		uint len : 31;
		uint isLong : 1;

		LongLenType(uint l = 0) : len(l), isLong(1) {
		}

		operator uint() const {
			return len;
		}
	};

	struct ShortLenType
	{
		byte len : 7;
		byte isLong : 1;

		ShortLenType(uint l = 0) : len(MaxShortSize - l), isLong(0) {
		}

		operator uint() const {
			return MaxShortSize - len;
		}
	};

	struct LongData
	{
		char *data;
		LongLenType length;

		LongData();
		LongData(const LongData &l);
		LongData(LongData &&l);
		LongData(const char *str, uint len);
	};

	struct ShortData
	{
		char data[sizeof(LongData) - 1];
		ShortLenType length;

		ShortData();
		ShortData(const ShortData &s);
		ShortData(const char *str, uint len);

		ShortData &operator=(const ShortData &s);
	};

	static_assert(sizeof(ShortData) == sizeof(LongData), "String2::LongData should be the same length as String2::ShortData");

	static constexpr uint MaxShortSize = sizeof(ShortData::data);

	public:
		using iterator = char *;
		using const_iterator = const char *;

		String2();
		String2(const String2 &str);
		String2(String2 &&str);
		String2(const char *str);
		String2(const char *str, uint len);

		uint size() const;
		bool isEmpty() const;

		void clear();

		bool isLong() const;

		bool beginsWith(const String2 &str) const;
		bool endsWith(const String2 &str) const;

		const_iterator find(const String2 &str, uint from = 0) const;
		iterator find(const String2 &str, uint from = 0);
		bool contains(const String2 &str) const;

		char *data();
		const char *data() const;

		iterator begin();
		iterator end();
		const_iterator begin() const;
		const_iterator end() const;
		const_iterator cbegin() const;
		const_iterator cend() const;

		void swap(String2 &str);

		char &operator[](uint i);
		char operator[](uint i) const;

		String2 &operator=(const String2 &str);
		String2 &operator=(String2 &&str);
		String2 &operator+=(const String2 &rhs);

		String2 operator+(const String2 &rhs) const;

		bool operator==(const String2 &str) const;
		bool operator!=(const String2 &str) const;
		bool operator<(const String2 &s) const;
		bool operator>(const String2 &s) const;

	private:
		union
		{
			LongData l;
			ShortData s;
		};

		static char *allocLong(uint len);
		static void freeLong(LongData &d);
};

}
}
#endif // N_CORE_STRING2_H
