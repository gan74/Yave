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

#include "SpinLock.h"

#if __has_include(<immintrin.h>)
#include <immintrin.h>
#define Y_ASM_PAUSE() _mm_pause()
#else
#define Y_ASM_PAUSE() do {} while(false)
#endif

namespace y {
namespace concurrent {

SpinLock::SpinLock() : _spin(Unlocked) {
}

SpinLock::~SpinLock() {
#ifdef Y_DEBUG
    const bool unlocked = _spin.exchange(Destroyed, std::memory_order_acquire) == Unlocked;
    unused(unlocked);
    y_debug_assert(unlocked);
#endif
}

void SpinLock::lock() {
    for(usize failed = 0; !try_lock(); ++failed) {
        wait_once();
    }
}

bool SpinLock::try_lock() {
    const Type res = _spin.exchange(Locked, std::memory_order_acquire);
    y_debug_assert(res != Destroyed);
    return res == Unlocked;
}

void SpinLock::unlock() {
    y_debug_assert(_spin != Destroyed);
    _spin.store(Unlocked, std::memory_order_release);
}

void SpinLock::wait_once() {
    Y_ASM_PAUSE();
}

}
}

#undef Y_ASM_PAUSE

