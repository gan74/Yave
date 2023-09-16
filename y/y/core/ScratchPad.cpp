/*******************************
Copyright (c) 2016-2023 Grégoire Angerand

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

static constexpr usize scratch_buffer_size = 64 * 1024;

static thread_local std::array<u8, scratch_buffer_size> scratch_buffer;
static thread_local u8* scratch_begin = scratch_buffer.data();

void* alloc_scratchpad(usize size) {
    if(!size) {
        return nullptr;
    }

    const usize aligned_size = align_up_to_max(size);
    const u8* scratch_end = scratch_buffer.data() + scratch_buffer.size();

    unused(scratch_end);
    y_debug_assert(scratch_begin >= scratch_buffer.data());
    y_debug_assert(scratch_begin <= scratch_end);

    u8* data = scratch_begin;
    scratch_begin += aligned_size;

    y_always_assert(scratch_begin <= scratch_end, "Scratch pad full: trying to allocate {} bytes (free space = {})", size, scratch_end - data);

    return data;
}


void free_scratchpad(void* ptr, usize size) {
    y_debug_assert(!ptr == !size);
    if(!ptr) {
        return;
    }

    const usize aligned_size = align_up_to_max(size);

    y_debug_assert(scratch_begin);

    u8* alloc_end = static_cast<u8*>(ptr) + aligned_size;
    unused(alloc_end);
    y_debug_assert(alloc_end == scratch_begin);

    scratch_begin = static_cast<u8*>(ptr);

#ifdef Y_DEBUG
    std::fill(scratch_begin, alloc_end, u8(0xFE));
#endif
}

}
}
}

