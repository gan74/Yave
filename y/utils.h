/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
**********************************/
#ifndef Y_UTILS_H
#define Y_UTILS_H

#include "defines.h"

#include <new>
#include <typeinfo>
#include <tuple>
#include <array>

#include <y/utils/types.h>

#include <y/utils/iterable.h>
#include <y/utils/collections.h>
#include <y/utils/sort.h>
#include <y/utils/hash.h>
#include <y/utils/log.h>
#include <y/utils/os.h>


namespace y {

struct Nothing {
	template<typename... Args>
	Nothing(Args...) {
	}

	template<typename... Args>
	Nothing operator()(Args...) const {
		return *this;
	}

	template<typename... Args>
	Nothing operator()(Args...) {
		return *this;
	}

	template<typename T>
	operator T() const {
		return fatal("y::Nothing used");
	}

	template<typename T>
	operator const T&() const {
		return fatal("y::Nothing used");
	}
};




constexpr usize log2ui(usize n) {
	return (n >> 1) ? log2ui(n >> 1) + 1 : 0;
}




template<typename... Args>
constexpr void unused(Args&&...) {}



template<typename T>
void do_not_destroy(T&& t) {
	union U {
		U() {}
		~U() {}
		std::remove_reference_t<T> t;
	} u;
	new(&u.t) std::remove_reference_t<T>(std::forward<T>(t));
}


constexpr bool is_64_bits() {
	return sizeof(void*) == 8;
}

constexpr bool is_32_bits() {
	return sizeof(void*) == 4;
}




namespace detail {
	static constexpr u32 endian = 0x01020304; // http://stackoverflow.com/questions/1583791/constexpr-and-endianness
	static constexpr u32 endianness = static_cast<const u8 &>(endian);
}

constexpr bool is_little_endian() {
	return detail::endianness == 0x04;
}

constexpr bool is_big_endian() {
	return detail::endianness == 0x01;
}

static_assert(is_little_endian() || is_big_endian(), "Unable to determine endianness");




namespace detail {

template<typename T>
class ScopeExit {
	public:
		ScopeExit(T&& t) : ex(std::forward<T>(t)) {
		}

		~ScopeExit() {
			ex();
		}

	private:
		T ex;
};

}

template<typename T>
auto scope_exit(T&& t) {
	return detail::ScopeExit<T>(std::forward<T>(t));
}

}


#endif
