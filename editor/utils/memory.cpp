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

#include "memory.h"

#include <y/utils/memory.h>

#include <atomic>
#include <cstdlib>

namespace editor {
namespace memory {

constinit std::atomic<usize> live_allocs = 0;
constinit std::atomic<u64> total_allocs = 0;

usize live_allocations() {
    return live_allocs;
}

u64 total_allocations() {
    return total_allocs;
}





static void* alloc_internal(usize size, usize alignment = max_alignment) {
    if(!size) {
        return nullptr;
    }

    auto try_alloc = [=] {
        #ifdef Y_MSVC
            return _aligned_malloc(size, alignment);
        #else
            return std::aligned_alloc(alignment, size);
        #endif
    };

    void* ptr = nullptr;
    while((ptr = try_alloc()) == nullptr) {
        std::new_handler nh = std::get_new_handler();
        if(!nh) {
            throw std::bad_alloc{};
        }
        nh();
    }

    ++total_allocs;
    ++live_allocs;

    y_profile_alloc(ptr, size);

    return ptr;

}

static void free_internal(void* ptr) {
    if(ptr) {
        --live_allocs;
    }

    y_profile_free(ptr);

#ifdef Y_MSVC
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}
}
}




void* operator new(std::size_t size) {
    return editor::memory::alloc_internal(size);
}


void* operator new(std::size_t size, std::align_val_t al) {
    return editor::memory::alloc_internal(size, std::size_t(al));
}



void operator delete(void* ptr) noexcept {
    editor::memory::free_internal(ptr);
}

void operator delete(void* ptr, std::align_val_t) noexcept {
    editor::memory::free_internal(ptr);
}

