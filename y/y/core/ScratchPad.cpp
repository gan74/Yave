/*******************************
Copyright (c) 2016-2022 Grégoire Angerand

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

#include "ScratchPad.h"
#include "FixedArray.h"

#include <y/utils/memory.h>
#include <y/utils/format.h>

namespace y {
namespace core {
namespace detail {

static constexpr usize scratch_buffer_size = 32 * 1024;

static thread_local FixedArray<u8> scratch_buffer;
static thread_local u8* scratch_begin = nullptr;

void* alloc_scratchpad(usize size) {
    if(!size) {
        return nullptr;
    }

    auto& scratch = scratch_buffer;
    auto& begin = scratch_begin;

    if(!begin) {
        scratch = FixedArray<u8>(scratch_buffer_size);
        begin = scratch.data();
    }

    const usize aligned_size = align_up_to_max(size);
    u8* data = begin;
    begin += aligned_size;

    y_always_assert(begin <= scratch.end(), "Scratch pad full: trying to allocate % bytes", size);

    return data;
}


void free_scratchpad(void* ptr, usize size) {
    y_debug_assert(!ptr == !size);
    if(!ptr) {
        return;
    }

    auto& begin = scratch_begin;
    y_debug_assert(begin);

    const usize aligned_size = align_up_to_max(size);

    u8* alloc_end = static_cast<u8*>(ptr) + aligned_size;
    y_debug_assert(alloc_end == begin);

    begin = static_cast<u8*>(ptr);

#ifdef Y_DEBUG
    std::fill(begin, alloc_end, 0xFE);
#endif
}

}
}
}

