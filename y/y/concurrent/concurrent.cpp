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
#include "concurrent.h"


#include "StaticThreadPool.h"

namespace y {
namespace concurrent {
namespace detail {
static thread_local const char* thread_name = nullptr;
}

StaticThreadPool& default_thread_pool() {
    static StaticThreadPool _pool;
    return _pool;
}

u32 thread_id() {
    static std::atomic<u32> id = 0;
    static thread_local u32 tid = ++id;
    return tid;
}

const char* set_thread_name(const char* thread_name) {
    // Force to generate id
    thread_id();

    const char* prev = detail::thread_name;
    detail::thread_name = thread_name;
    return prev;
}

const char* thread_name() {
    return detail::thread_name;
}

}
}

