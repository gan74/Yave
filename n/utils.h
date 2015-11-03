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

namespace n {
namespace graphics {
	namespace gl {}
}
namespace concurrent {}
namespace signals {}
namespace assets {}
namespace core {}
namespace math {}
namespace io {}
}

namespace n {

static constexpr void *null = 0;

class NonCopyable
{
	public:
		NonCopyable() {}
		NonCopyable(const NonCopyable &) = delete;
		NonCopyable &operator=(const NonCopyable &) = delete;
};

uint uniqueId();

constexpr uint log2ui(uint n) {
	return (n >> 1) ? log2ui(n >> 1) + 1 : 0;
}


constexpr bool is64Bits() {
	return sizeof(void *) == 8;
}

namespace internal {
	static constexpr uint32 endian = 0x01020304; // http://stackoverflow.com/questions/1583791/constexpr-and-endianness
	static constexpr uint32 endianness = (const byte&)endian;
}

constexpr bool isLittleEndian() {
	return internal::endianness == 0x04;
}

constexpr bool isBigEndian() {
	return internal::endianness == 0x01;
}

static_assert(isLittleEndian() || isBigEndian(), "Unable to determine endianness !");


}

#endif // N_UTILS_H
