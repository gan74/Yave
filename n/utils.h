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

#ifndef N_UTILS_H
#define N_UTILS_H

#include <n/utils/alloc.h>
#include <n/utils/hash.h>
#include <n/utils/math.h>
#include <n/utils/random.h>
#include <n/utils/logs.h>
#include <n/utils/perf.h>
#include <n/utils/SysInfo.h>


#define N_SCOPE_LINE_HELPER(CODE, LINE) auto _scope_exit_at_ ## LINE = n::scopeExit([&]() { CODE; })
#define N_SCOPE_HELPER(CODE, LINE) N_SCOPE_LINE_HELPER(CODE, LINE)
#define N_SCOPE(CODE) N_SCOPE_HELPER(CODE, __LINE__)

namespace n {
namespace graphics {}
namespace concurrent {}
namespace signals {}
namespace assets {}
namespace core {}
namespace math {}
namespace io {}
}

namespace n {

static constexpr void *null = 0;

uint uniqueId();

constexpr uint log2ui(uint n) {
	return (n >> 1) ? log2ui(n >> 1) + 1 : 0;
}


constexpr bool is64Bits() {
	return sizeof(void *) == 8;
}

constexpr bool is32Bits() {
	return sizeof(void *) == 4;
}

namespace details {
	static constexpr uint32 endian = 0x01020304; // http://stackoverflow.com/questions/1583791/constexpr-and-endianness
	static constexpr uint32 endianness = (const byte&)endian;
}

constexpr bool isLittleEndian() {
	return details::endianness == 0x04;
}

constexpr bool isBigEndian() {
	return details::endianness == 0x01;
}

static_assert(isLittleEndian() || isBigEndian(), "Unable to determine endianness !");
static_assert(isLittleEndian(), "This was designed for little endian systems, it may work on other architectures.");

template<typename T>
class ScopeExit
{
	public:
		ScopeExit(T t) : ex(t) {
		}

		~ScopeExit() {
			ex();
		}

	private:
		T ex;
};

template<typename T>
ScopeExit<T> scopeExit(T t) {
	return ScopeExit<T>(t);
}

}

#endif // N_UTILS_H
