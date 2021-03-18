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
#ifndef Y_UTILS_HASH_H
#define Y_UTILS_HASH_H

#include <y/defines.h>

#include "types.h"
#include "name.h"

#include <functional>

namespace y {

// from boost
template<typename T>
inline constexpr void hash_combine(T& seed, T value) {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}

template<typename T>
inline constexpr u64 ct_type_hash() {
    u64 hash = 0xd5a7de585d2af52b;
    for(char c : ct_type_name<T>()) {
        hash_combine(hash, u64(c));
    }
    return hash;
}


template<typename T>
inline auto hash(const T& t) {
    return std::hash<T>()(t);
}

template<typename B, typename E>
inline auto hash_range(B begin, const E& end) {
    decltype(hash(*begin)) h = 0;
    for(; begin != end; ++begin) {
        hash_combine(h, hash(*begin));
    }
    return h;
}

template<typename C>
inline auto hash_range(const C& c) {
    return hash_range(std::begin(c), std::end(c));
}

}

namespace std {
template<typename A, typename B>
struct hash<std::pair<A, B>> : hash<A>, hash<B> {
    auto operator()(const std::pair<A, B>& p) const {
        auto a = hash<A>::operator()(p.first);
        y::hash_combine(a, hash<B>::operator()(p.second));
        return a;
    }
};
}

#endif // Y_UTILS_HASH_H

