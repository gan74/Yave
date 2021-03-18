/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include <y/utils/types.h>

#define y_defer(expr) auto y_create_name_with_prefix(defer) = y::ScopeExit([&]() { expr; })

namespace y {

struct Empty {};


template<typename T>
constexpr T log2ui(T n) {
    return (n >> 1) ? log2ui(n >> 1) + 1 : 0;
}




template<typename... Args>
constexpr void unused(Args&&...) {}



inline constexpr bool is_64_bits() {
    return sizeof(void*) == 8;
}

inline constexpr bool is_32_bits() {
    return sizeof(void*) == 4;
}




namespace detail {
    static constexpr u32 endian = 0x01020304; // http://stackoverflow.com/questions/1583791/constexpr-and-endianness
    static constexpr u32 endianness = static_cast<const u8&>(endian);
}

inline constexpr bool is_little_endian() {
    return detail::endianness == 0x04;
}

inline constexpr bool is_big_endian() {
    return detail::endianness == 0x01;
}

static_assert(is_little_endian() || is_big_endian(), "Endianness unknown");





template<typename T>
class ScopeExit {
    public:
        inline ScopeExit(T&& t) : _ex(y_fwd(t)) {
        }

        inline ScopeExit(ScopeExit&& other) : _ex(std::move(other._ex)) {
        }

        inline ~ScopeExit() {
            _ex();
        }

    private:
        T _ex;
};

}


#endif

