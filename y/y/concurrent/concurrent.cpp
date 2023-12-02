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
#include "concurrent.h"

#include "StaticThreadPool.h"

#ifdef Y_OS_WIN
#include <windows.h>
#endif

namespace y {
namespace concurrent {
namespace detail {
static thread_local char thread_name[64] = {};
}

u32 thread_id() {
    static std::atomic<u32> id = 0;
    static thread_local u32 tid = ++id;
    return tid;
}

void set_thread_name(const char* thread_name) {
    // Force to generate id
    thread_id();

    usize len = std::min(std::strlen(thread_name), sizeof(detail::thread_name) - 1);
    std::copy_n(thread_name, len, detail::thread_name);

#ifdef Y_OS_WIN
    if(auto set_thread_desc = reinterpret_cast<decltype(&::SetThreadDescription)>(::GetProcAddress(::GetModuleHandleA("Kernel32.dll"), "SetThreadDescription"))) {
        wchar_t w_thread_name[sizeof(detail::thread_name)] = {};
        std::copy_n(thread_name, len, w_thread_name);
        set_thread_desc(::GetCurrentThread(), w_thread_name);
    }
#endif
}

const char* thread_name() {
    return detail::thread_name;
}

}
}

