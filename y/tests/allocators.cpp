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
#include <y/test/test.h>
#include <y/mem/allocators.h>
#include <y/core/Vector.h>

namespace {
using namespace y;
using namespace memory;

/*y_test_func("StackBlockAllocator basic") {
    static constexpr usize size = align_up_to_max(1024);
    StackBlockAllocator<size, Mallocator> allocator;
    y_test_assert(allocator.allocate(size + 1) == nullptr);

    {
        void* p = allocator.allocate(size);
        y_test_assert(p);
        y_test_assert(allocator.allocate(4) == nullptr);
        allocator.deallocate(p, size);
    }

    {
        void* a = allocator.allocate(size / 2 - 2);
        void* b = allocator.allocate(size / 2);
        y_test_assert(a);
        y_test_assert(b);
        y_test_assert(allocator.allocate(1) == nullptr);
        allocator.deallocate(b, size / 2);
        allocator.deallocate(a, size / 2 - 1);
    }
}

y_test_func("FixedSizeFreeListAllocator basic") {
    static constexpr usize size = align_up_to_max(std::max(usize(8), max_alignment));
    static constexpr usize min_size = size - max_alignment + 1;
    //static_assert(align_up_to_max(size) == size);

    FixedSizeFreeListAllocator<size, Mallocator> allocator;

    y_test_assert(allocator.allocate(size + 1) == nullptr);
    void* p1 = allocator.allocate(size);
    void* p2 = allocator.allocate(min_size);
    y_test_assert(p1);
    y_test_assert(p2);
    y_test_assert(p1 != p2);

    allocator.deallocate(p1, size);
    void* p3 = allocator.allocate(size - 1);
    y_test_assert(p3 == p1);

    allocator.deallocate(p3, size - 1);
    allocator.deallocate(p2, min_size);
}*/
}

