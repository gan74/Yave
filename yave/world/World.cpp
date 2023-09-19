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

#include "World.h"

namespace yave {
namespace detail {
u32 create_node_type() {
    static std::atomic<u32> type_index = 0;
    return type_index++;
}


void* alloc_page(usize size, usize align) {
    void* ptr = nullptr;
#ifdef Y_MSVC
    ptr = _aligned_malloc(size, align);
#else
    ptr = std::aligned_alloc(align, size);
#endif

    y_debug_assert(ptr);
    return ptr;
}

void dealloc_page(void* ptr, usize size) {
    unused(size);
#ifdef Y_MSVC
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

}


NodeContainerBase::NodeContainerBase(NodeType type) : _type(type) {
}

NodeContainerBase::~NodeContainerBase() {

}

NodeType NodeContainerBase::type() const {
    return _type;
}




}

