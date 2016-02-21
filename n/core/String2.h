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
		String2();
		String2(const String2 &str);
		String2(String2 &&str);
		String2(const char *str);
		String2(const char *str, uint len);

		uint size() const;
		bool isLong() const;

		char *data();
		const char *data() const;

		void swap(String2 &&str);

		String2 &operator=(const String2 &str);
		String2 &operator=(String2 &&str);

		String2 operator+(const String2 &rhs) const;

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
