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
#ifndef Y_DEFINES_H
#define Y_DEFINES_H


namespace y {
[[noreturn]] void fatal(const char* msg, const char* file = nullptr, int line = 0);
void break_in_debugger();
}



#define Y_TODO(...) /* __VA_ARGS__ */

#if defined(Y_NO_DEBUG) && defined(Y_DEBUG)
#undef Y_DEBUG
#endif

// keep the namespacing ?
#define y_fatal(msg) y::fatal((msg), __FILE__, __LINE__)

#define y_always_assert(cond, ...) do { if(!(cond)) [[unlikely]] { y_fatal(__VA_ARGS__); } } while(false)

#ifdef Y_DEBUG
#define y_debug_assert(cond) y_always_assert(cond, "Assert failed: " #cond)
#else
#define y_debug_assert(cond) do { /*(void)(cond);*/ } while(false)
#endif

#define y_fwd(var) std::forward<decltype(var)>(var)

namespace y {
#ifdef Y_DEBUG
static constexpr bool is_debug_defined = true;
#else
static constexpr bool is_debug_defined = false;
#endif
}


/****************** OS DEFINES BELOW ******************/

#if defined(WIN32) || defined(__WIN32) || defined(__WIN32__) || defined(_WINDOWS)
#define Y_OS_WIN
#define WIN32_LEAN_AND_MEAN

#ifndef NOMINMAX
#define NOMINMAX
#endif

#endif

#if defined(__linux__) || defined(__gnu_linux__)
#define Y_OS_LINUX
#endif

#ifdef _MSC_VER
#define Y_MSVC
// Disable warning about "defined" in macro expansion (happens in windows.h)
#pragma warning(disable : 5105)
#endif

#if defined(__clang__)
#define Y_CLANG
#elif defined(__GNUC__)
#define Y_GCC
#endif

#ifdef __GNUC__
#define Y_GNU
#endif



#if defined(_M_IX86) || defined(_M_X64) || defined(__i386__) || defined(__x86_64__)
#define Y_SSE
#else
#warning SSE not supported
#endif



/****************** UTILS DEFINE ******************/

#define Y_CREATE_NAME_LINE_HELPER(prefix, LINE) _generated_ ## prefix ## _at_ ## LINE
#define Y_CREATE_NAME_HELPER(prefix, LINE) Y_CREATE_NAME_LINE_HELPER(prefix, LINE)

#define y_create_name_with_prefix(prefix) Y_CREATE_NAME_HELPER(prefix, __LINE__)
#define y_create_name y_create_name_with_prefix()


#ifdef Y_DEBUG
#define y_breakpoint do { y::break_in_debugger(); } while(false)
#else
#define y_breakpoint do {} while(false)
#endif


#ifdef Y_MSVC
#define Y_DLL_EXPORT __declspec(dllexport)
#define Y_DLL_IMPORT __declspec(dllimport)
#else
#define Y_DLL_EXPORT
#define Y_DLL_IMPORT
#endif


#ifdef Y_DEBUG
#define y_assume(cond) y_debug_assert(cond)
#else // Y_DEBUG
#if defined(Y_MSVC)
#define y_assume(cond) __assume(cond)
#elif defined(Y_GNU)
#define y_assume(cond) do { if(!(cond)) { __builtin_unreachable(); } } while(false)
#else
#define y_assume(cond) do {} while(false && (cond))
#endif 
#endif //Y_DEBUG

#if defined(Y_MSVC)
#define y_force_inline __forceinline
#elif defined(Y_GNU)
#define y_force_inline inline __attribute__((__always_inline__))
#else
#define y_force_inline inline
#endif


#ifdef Y_DEBUG
#define y_unreachable() y_fatal("y_unreachable called")
#else // Y_DEBUG
#if defined(Y_MSVC)
#define y_unreachable() __assume(false)
#elif defined(Y_GNU)
#define y_unreachable() __builtin_unreachable()
#else
#define y_unreachable() y_fatal("y_unreachable called")
#endif
#endif //Y_DEBUG


#endif // Y_DEFINES_H
