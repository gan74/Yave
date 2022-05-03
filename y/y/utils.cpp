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

#include "utils.h"

#include <y/utils/log.h>
#include <y/concurrent/concurrent.h>

#include <array>
#include <cstdio>
#include <cstdlib>
#include <csignal>

#ifdef Y_OS_WIN
#include <windows.h>
#endif

#ifdef Y_OS_LINUX
#include <sys/ptrace.h>
#endif

namespace y {

#ifdef Y_DEBUG
namespace core::result {
bool break_on_error = false;
}
#endif


void break_in_debugger() {
#ifdef Y_OS_WIN
    if(IsDebuggerPresent()) {
        DebugBreak();
    }
#endif
#ifdef Y_OS_LINUX
    bool debugger_present = false;
    if(ptrace(PTRACE_TRACEME, 0, 1, 0) < 0) {
        debugger_present = true;
    } else {
        ptrace(PTRACE_DETACH, 0, 1, 0);
    }
    if(debugger_present) {
        std::raise(SIGTRAP);
    }
#endif
}


void fatal(const char* msg, const char* file, int line) {
    // Don't use fmt since it can assert
    std::array<char, 1024> buffer = {};
    std::snprintf(buffer.data(), buffer.size(), "%s", msg);

    if(file) {
        const auto tmp_buffer = buffer;
        std::snprintf(buffer.data(), buffer.size(), "%s in file \"%s\"", tmp_buffer.data(), file);
    }
    if(line) {
        const auto tmp_buffer = buffer;
        std::snprintf(buffer.data(), buffer.size(), "%s at line %i", tmp_buffer.data(), line);
    }

    if(const char* thread_name = concurrent::thread_name()) {
        const auto tmp_buffer = buffer;
        std::snprintf(buffer.data(), buffer.size(), "%s on thread \"%s\"", tmp_buffer.data(), thread_name);
    }

    log_msg(buffer.data(), Log::Error);
    y_breakpoint;
    std::abort();
}

}
