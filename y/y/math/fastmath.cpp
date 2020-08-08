/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include "fastmath.h"

#include <cmath>

#ifndef Y_NO_INTRINSICS
#if __has_include(<xmmintrin.h>) && __has_include(<smmintrin.h>) && __has_include(<emmintrin.h>)
#include <xmmintrin.h>
#include <smmintrin.h>
#include <emmintrin.h>
#define Y_USE_INTRINSICS
#endif
#endif

namespace y {
namespace math {

template<typename T>
static inline is_aligned(T* x) {
    static_assert(16 % sizeof(T) == 0);
    return std::uintptr_t(x) % 16 == 0;
}

template<typename T>
static inline T* align_begin(T* x) {
    y_debug_assert(std::uintptr_t(x) % sizeof(T) == 0);
    while(!is_aligned(x)) {
        ++x;
    }
    return x;
}

template<typename T>
static inline T* align_end(T* x) {
    y_debug_assert(std::uintptr_t(x) % sizeof(T) == 0);
    while(!is_aligned(x)) {
        --x;
    }
    return x;
}


// https://gist.github.com/Novum/1200562/3847ad4e522ba8d30e63357cc4c405ef262f26de
void fast_pow(float* x, usize count, float y) {
#ifdef Y_USE_INTRINSICS
    const float* end = x + count;
    const float* aligned_begin = align_begin(x);
    const float* aligned_end = align_end(end);

    while(x != aligned_begin) {
        *x = std::pow(*x, y);
        ++x;
    }

    static const __m128 yy = _mm_set1_ps(y);
    static const __m128 ones = _mm_set1_ps(1.0f);
    static const __m128 halfs = _mm_set1_ps(0.5f);
    for(; x < aligned_end; x += 4) {
        __m128 xx = _mm_load_ps(x);
        __m128 a = _mm_sub_ps(ones, yy);
        __m128 b = _mm_sub_ps(xx, ones);
        __m128 a2 = _mm_mul_ps(a, a);
        __m128 b2 = _mm_mul_ps(b, b);
        __m128 c = _mm_mul_ps(halfs, b2);
        __m128 d = _mm_sub_ps(b, c);
        __m128 d2 = _mm_mul_ps(d, d);
        __m128 e = _mm_mul_ps(a2, d2);
        __m128 f = _mm_mul_ps(a, d);
        __m128 g = _mm_mul_ps(halfs, e);
        __m128 h = _mm_add_ps(ones, f);
        __m128 i = _mm_add_ps(h, g);
        __m128 i_rcp = _mm_rcp_ps(i);
        _mm_store_ps(x, _mm_mul_ps(xx, i_rcp));
    }

    while(x != end) {
        *x = std::pow(*x, y);
        ++x;
    }
#else
    for(usize i = 0; i != count; ++i) {
        x[i] = std::pow(x[i], y);
    }
#endif
}


// https://stackoverflow.com/questions/12121640/how-to-load-a-pixel-struct-into-an-sse-register
void fast_unpack_unorm(const u8* input, usize count, float* output) {
    static const float inv_max_u8 = 1.0f / 255.0f;
#ifdef Y_USE_INTRINSICS
    const float* end = output + count;
    const float* aligned_begin = align_begin(output);
    const float* aligned_end = align_end(end);

    while(output != aligned_begin) {
        *output++ = *input++ * inv_max_u8;
    }

    static const __m128 n = _mm_set1_ps(inv_max_u8);
    for(; output < aligned_end; output += 4, input += 4) {
        __m128i i = _mm_cvtsi32_si128(*reinterpret_cast<const i32*>(input));
        __m128i i_16 = _mm_unpacklo_epi8(i, _mm_setzero_si128());
        __m128i i_32 = _mm_unpacklo_epi16(i_16, _mm_setzero_si128());
        // or __m128i i_32 = _mm_cvtepu8_epi32(i); if we have SSE4.1
        __m128 f = _mm_cvtepi32_ps(i_32);
        _mm_store_ps(output, _mm_mul_ps(f, n));
    }

    while(output != end) {
        *output++ = *input++ * inv_max_u8;
    }
#else
    for(usize i = 0; i != count; ++i) {
        output[i] = input[i] * inv_max_u8;
    }
#endif
}

void fast_pack_unorm(const float* input, usize count, u8* output) {
    static const float max_u8 = 255.0f;
#ifdef Y_USE_INTRINSICS
    const float* end = input + count;
    const float* aligned_begin = align_begin(input);
    const float* aligned_end = align_end(end);

    while(input != aligned_begin) {
        *output++ = u8(*input++ * max_u8);
    }

    static const __m128 n = _mm_set1_ps(max_u8);
    //static const __m128 ones = _mm_set1_ps(1.0f);
    for(; input < aligned_end; input += 4, output += 4) {
        __m128 f = _mm_load_ps(input);
        // __m128 ma = _mm_max_ps(f, ones);
        __m128 s = _mm_mul_ps(f, n);
        __m128i i_32 = _mm_cvtps_epi32(s);
        __m128i i_16 = _mm_packs_epi32(i_32, _mm_setzero_si128()); // packs instead of packus?
        __m128i i = _mm_packus_epi16(i_16, _mm_setzero_si128());
        *reinterpret_cast<i32*>(output) = _mm_cvtsi128_si32(i);
    }

    while(input != end) {
        *output++ = u8(*input++ * max_u8);
    }
#else
    for(usize i = 0; i != count; ++i) {
        output[i] = u8(input[i] * max_u8);
    }
#endif
}


}
}


