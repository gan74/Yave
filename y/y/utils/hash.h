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
#ifndef Y_UTILS_HASH_H
#define Y_UTILS_HASH_H

#include <y/defines.h>

#include "types.h"
#include "name.h"

#include <functional>

namespace y {

inline constexpr usize hash_u64(u64 x)  {
    x ^= x >> 33u;
    x *= UINT64_C(0xFF51AFD7ED558CCD);
    x ^= x >> 33u;
    return static_cast<usize>(x);
}


#ifdef Y_USE_STD_HASH
template<typename T>
using Hash = std::hash<T>;
#else
// Custom hash to avoid std::hash implementation that don't do anything for integers
namespace detail {
template<typename T>
static constexpr bool use_custom_hash_v = std::is_integral_v<T> && sizeof(T) <= sizeof(u64);
}

template<typename T>
struct Hash{};

template<typename T> requires(!detail::use_custom_hash_v<T>)
struct Hash<T> : std::hash<T> {
    usize operator()(const T& h) const {
        return std::hash<T>::operator()(h);
    }
};

template<typename T> requires(detail::use_custom_hash_v<T>)
struct Hash<T> {
    usize operator()(const T& h) const {
        return hash_u64(static_cast<u64>(h));
    }
};
#endif

template<typename T>
inline constexpr usize hash(const T& t) {
    return Hash<T>()(t);
}



// from boost
template<typename T>
inline constexpr void hash_combine(T& seed, T value) {
    seed ^= value + 0x9e3779b9 + (seed << 6) + (seed >> 2);
}



template<typename B, typename E>
inline constexpr usize hash_range(B begin, const E& end) {
    decltype(hash(*begin)) h = 0;
    for(; begin != end; ++begin) {
        hash_combine(h, hash(*begin));
    }
    return h;
}

template<typename C>
inline constexpr usize hash_range(const C& c) {
    return hash_range(std::begin(c), std::end(c));
}

template<typename T>
inline constexpr u64 ct_type_hash() {
    u64 hash = 0xd5a7de585d2af52b;
    for(char c : ct_type_name<T>()) {
        hash_combine(hash, u64(c));
    }
    return hash;
}

inline constexpr u32 ct_str_hash(std::string_view str) {
    u32 hash = 0xec81fb49;
    for(char c : str) {
        hash_combine(hash, u32(c));
    }
    return hash;
}

template<typename T>
static constexpr u64 ct_type_hash_v = ct_type_hash<T>();

template<typename T>
struct RangeHash {
    usize operator()(const T& h) const {
        return hash_range(h);
    }
};

}

template<typename A, typename B>
struct std::hash<std::pair<A, B>> : std::hash<A>, std::hash<B> {
    auto operator()(const std::pair<A, B>& p) const {
        auto a = hash<A>::operator()(p.first);
        y::hash_combine(a, hash<B>::operator()(p.second));
        return a;
    }
};

#endif // Y_UTILS_HASH_H

