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

#include <y/test/test.h>

#include <y/math/math.h>
#include <y/math/fastmath.h>

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace {
using namespace y;
using namespace y::math;

static const usize start_offset = 7;
static const usize runs_size = 240;

y_test_func("fast_unpack_unorm") {
    u8 unorms[256];
    for(usize i = 0; i != 256; ++i) {
        unorms[i] = i;
    }

    float unpacked[256] = {};

    fast_unpack_unorm(unorms + start_offset, runs_size, unpacked + start_offset);

    for(usize i = 0; i != start_offset; ++i) {
        y_test_assert(unpacked[i] == 0.0f);
    }
    for(usize i = start_offset + runs_size; i != 256; ++i) {
        y_test_assert(unpacked[i] == 0.0f);
    }
    for(usize i = start_offset; i != start_offset + runs_size; ++i) {
        const float ref = i / 255.0f;
        y_test_assert(std::abs(ref - unpacked[i]) < 0.001f);
    }
}

y_test_func("fast_pack_unorm") {
    float unpacked[256];
    for(usize i = 0; i != 256; ++i) {
        unpacked[i] = i / 255.0f;
    }

    u8 unorms[256] = {};
    fast_pack_unorm(unpacked + start_offset, runs_size, unorms + start_offset);
    for(usize i = 0; i != start_offset; ++i) {
        y_test_assert(unorms[i] == 0);
    }
    for(usize i = start_offset + runs_size; i != 256; ++i) {
        y_test_assert(unorms[i] == 0);
    }
    for(usize i = start_offset; i != start_offset + runs_size; ++i) {
        y_test_assert(unorms[i] == i);
    }
}

#if 0
y_test_func("fast_pow_01") {
    const float y = 2.7f;

    float data[256];
    for(usize i = 0; i != 256; ++i) {
        data[i] = i / 255.0f;
    }
    fast_pow_01(data, 256, y);
    for(usize i = start_offset; i != start_offset + runs_size; ++i) {
        const float ref = std::pow(i / 255.0f, y);
        log_msg(fmt("%) % => % != %", i, i / 255.0f, ref, data[i]));
    }

    /*fast_pow_01(data + start_offset, runs_size, y);

    for(usize i = 0; i != start_offset; ++i) {
        y_test_assert(data[i] == (i / 255.0f));
    }
    for(usize i = start_offset + runs_size; i != 256; ++i) {
        y_test_assert(data[i] == (i / 255.0f));
    }

    for(usize i = start_offset; i != start_offset + runs_size; ++i) {
        const float ref = std::pow(i / 255.0f, y);
        y_test_assert(std::abs(ref - data[i]) < 0.001f);
    }*/
}

y_test_func("fast_exp") {
    float data[256];
    for(usize i = 0; i != 256; ++i) {
        data[i] = i / 71.0f;
    }

    fast_exp(data + start_offset, runs_size);

    for(usize i = 0; i != start_offset; ++i) {
        y_test_assert(data[i] == (i / 71.0f));
    }
    for(usize i = start_offset + runs_size; i != 256; ++i) {
        y_test_assert(data[i] == (i / 71.0f));
    }

    for(usize i = start_offset; i != start_offset + runs_size; ++i) {
        const float x = i / 71.0f;
        const float ref = std::exp(x);
        log_msg(fmt("%) % => % != %", i, x, ref, data[i]));
        y_test_assert(std::abs(ref - data[i]) < (1.0f + x) * 0.01f);
    }
}
#endif

}

