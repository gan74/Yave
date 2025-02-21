/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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


#define y_defer_named(expr, name)   auto y_create_name_with_prefix(defer_ ## name) = y::ScopeGuard([&] { expr; })
#define y_defer(expr)               auto y_create_name_with_prefix(defer)          = y::ScopeGuard([&] { expr; })

#define y_only_once(expr)           y::only_once([&] { expr; })


namespace y {

struct Empty {};


template<typename T>
constexpr T log2ui(T n) {
    return (n >> 1) ? log2ui(n >> 1) + 1 : 0;
}

template<typename T>
constexpr T next_pow_of_2(T n) {
    const T l = log2ui(n);
    const T prev = T(1) << l;
    return prev < n ? prev * 2 : prev;
}

template<typename T>
constexpr bool is_pow_of_2(T n) {
    return (n & (n - 1)) == 0;
}



template<typename... Args>
constexpr void unused(Args&&...) {}



inline constexpr bool is_64_bits() {
    return sizeof(void*) == 8;
}

inline constexpr bool is_32_bits() {
    return sizeof(void*) == 4;
}

template<auto T>
inline consteval auto force_ct() {
    return T;
}

// https://stackoverflow.com/a/57453713/3496382

template<typename T>
class ScopeGuard : NonMovable {
    public:
        inline ScopeGuard(T&& t) : _ex(y_fwd(t)) {
        }

        inline ~ScopeGuard() {
            _ex();
        }

    private:
        T _ex;
};

template<typename F>
inline void only_once(F&& f) {
    static bool done_once = false;
    if(!done_once) {
        done_once = true;
        f();
    }
}

}


#endif

