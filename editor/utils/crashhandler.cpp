/*******************************
Copyright (c) 2016-2021 Gr�goire Angerand

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

#include "crashhandler.h"

#include <cstdlib>
#include <cstdio>
#include <csignal>

#if defined(Y_OS_WIN) && defined(__argv) && __has_include(<dbghelp.h>)
#if !defined(EDITOR_NO_CRASH_HANDLER)
#define EDITOR_WIN_CRASH_HANLDER
#endif
#endif


#ifdef EDITOR_WIN_CRASH_HANLDER
#include <windows.h>
#include <dbghelp.h>
#endif

namespace editor {
namespace crashhandler {

#ifdef EDITOR_WIN_CRASH_HANLDER
static decltype(&::SymInitialize) sym_init = nullptr;
static decltype(&::SymCleanup) sym_cleanup = nullptr;
static decltype(&::SymFunctionTableAccess64) func_table = nullptr;
static decltype(&::SymGetModuleBase64) module_base = nullptr;
static decltype(&::StackWalk64) stack_walk = nullptr;


static int print_addr(const void* ptr) {
#ifdef Y_MSVC
    return 0;
#else
    char buffer[512] = {};
    std::sprintf(buffer, "addr2line -f -p -s -C -e %s %p", __argv[0], ptr);
    return ::system(buffer);
#endif
}

// https://gist.github.com/jvranish/4441299
// http://theorangeduck.com/page/printing-stack-trace-mingw
static void print_stacktrace() {
    HANDLE process = ::GetCurrentProcess();

    CONTEXT context = {};
    context.ContextFlags = CONTEXT_FULL;
    ::RtlCaptureContext(&context);

    sym_init(process, nullptr, true);

    STACKFRAME64 stackframe = {};
    stackframe.AddrPC.Offset = context.Rip;
    stackframe.AddrPC.Mode = AddrModeFlat;
    stackframe.AddrFrame.Offset = context.Rsp;
    stackframe.AddrFrame.Mode = AddrModeFlat;
    stackframe.AddrStack.Offset = context.Rsp;
    stackframe.AddrStack.Mode = AddrModeFlat;

    while(stack_walk(IMAGE_FILE_MACHINE_AMD64, process, ::GetCurrentThread(), &stackframe, &context, nullptr, func_table, module_base, nullptr)) {
        print_addr(reinterpret_cast<const void*>(stackframe.AddrPC.Offset));
    }

    sym_cleanup(process);
}

static void handler(int sig) {
    // calling basically anything here is UB but we don't really care since we already crashed

    y_breakpoint;
    if(sig == SIGABRT) {
        std::printf("Program has aborted, dumping stack:\n");
    } else {
        std::printf("Program has crashed, dumping stack:\n");
    }
    print_stacktrace();
}

bool setup_handler() {
    if(HMODULE img_help = ::LoadLibrary("dbghelp.dll")) {
        sym_init = reinterpret_cast<decltype(sym_init)>(reinterpret_cast<void*>(::GetProcAddress(img_help, "SymInitialize")));
        sym_cleanup = reinterpret_cast<decltype(sym_cleanup)>(reinterpret_cast<void*>(::GetProcAddress(img_help, "SymCleanup")));
        func_table = reinterpret_cast<decltype(func_table)>(reinterpret_cast<void*>(::GetProcAddress(img_help, "SymFunctionTableAccess64")));
        module_base = reinterpret_cast<decltype(module_base)>(reinterpret_cast<void*>(::GetProcAddress(img_help, "SymGetModuleBase64")));
        stack_walk = reinterpret_cast<decltype(stack_walk)>(reinterpret_cast<void*>(::GetProcAddress(img_help, "StackWalk64")));
    }
    if(sym_init && sym_cleanup && func_table && module_base && stack_walk) {
        std::signal(SIGSEGV, handler);
        std::signal(SIGILL, handler);
        std::signal(SIGABRT, handler);
        return true;
    }
    return false;
}

#else
bool setup_handler() {
    return false;
}
#endif


}
}

