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

#include "memory.h"
#include "allocators.h"

#include <y/utils/log.h>
#include <y/utils/format.h>

namespace y {
namespace memory {

template<typename Allocator>
struct PrintingAllocator : private Allocator {
    [[nodiscard]] void* allocate(usize size) noexcept {
        log_msg(fmt("alloc(%)", size));
        return Allocator::allocate(size);
    }

    void deallocate(void* ptr, usize size) noexcept {
        log_msg(fmt("free(%)", size));
        Allocator::deallocate(ptr, size);
    }
};

using GlobalAllocatorType = ThreadSafeAllocator<LeakDetectorAllocator<Mallocator>>;

PolymorphicAllocatorBase* global_allocator() {
    static PolymorphicAllocator<GlobalAllocatorType> allocator;
    return &allocator;
}

PolymorphicAllocatorBase* thread_local_allocator() {
    static thread_local PolymorphicAllocator<GlobalAllocatorType> allocator;
    return &allocator;
}


[[nodiscard]] void* GlobalAllocator::allocate(usize size) noexcept {
    return global_allocator()->allocate(size);
}

void GlobalAllocator::deallocate(void* ptr, usize size) noexcept {
    global_allocator()->deallocate(ptr, size);
}

[[nodiscard]] void* ThreadLocalAllocator::allocate(usize size) noexcept {
    return thread_local_allocator()->allocate(size);
}

void ThreadLocalAllocator::deallocate(void* ptr, usize size) noexcept {
    thread_local_allocator()->deallocate(ptr, size);
}

}
}

