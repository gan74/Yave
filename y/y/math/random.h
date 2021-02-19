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
#ifndef Y_MATH_RANDOM_H
#define Y_MATH_RANDOM_H

#include <y/utils.h>

namespace y {
namespace math {

#if 0
class FastRandom {
    public:
        FastRandom(u64 seed = 0xdeadbeef) {
        }

        u32 operator()() {
            return (_seed += _seed * _seed | 5) >> 32;
        }

    private:
        u64 _seed;
};
#endif


#if 1
class FastRandom {
    public:
        using result_type = u32;

        constexpr FastRandom(u32 seed = 0xdeadbeef) : _a(0xf1ea5eed), _b(seed), _c(seed), _d(seed) {
            for(usize i = 0; i != 20; ++i) {
                (*this)();
            }
        }

        constexpr u32 operator()() {
            const u32 e = _a - rot(_b, 27);
            _a = _b ^ rot(_c, 17);
            _b = _c + _d;
            _c = _d + e;
            _d = e + _a;
            return _d;
        }

        static constexpr u32 max() {
            return u32(-1);
        }

        static constexpr u32 min() {
            return 0;
        }

    private:
        static constexpr u32 rot(u32 x, u32 k) {
            return (((x) << (k)) | ((x) >> (32 - (k))));
        }

        u32 _a;
        u32 _b;
        u32 _c;
        u32 _d;
};
#endif

}
}

#endif // Y_MATH_RANDOM_H

