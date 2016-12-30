/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

This code is licensed under the MIT License (MIT).

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
#ifndef Y_UTILS_OS_WIN_H
#define Y_UTILS_OS_WIN_H

#include <y/defines.h>
#include "types.h"

#ifdef Y_OS_WIN
#include <windows.h>

#ifndef Y_NO_PSAPI
#include <psapi.h>
#endif

namespace y {
namespace windows {

#define Y_WIN_DYN_FUNC(lib, func, func_ptr_type)																					\
auto func ## _func() {																												\
	static HMODULE module = nullptr;																								\
	static func_ptr_type = nullptr;																									\
	static bool is_init = false;																									\
	if(!is_init) {																													\
		is_init = true;																												\
		if((module = LoadLibrary(TEXT(#lib)))) {																					\
			func_ptr = reinterpret_cast<decltype(func_ptr)>(GetProcAddress(module, #func));											\
		}																															\
	}																																\
	return func_ptr;																												\
}

u64 filetime_to_ns(FILETIME time) {
	return ((u64(time.dwHighDateTime) << 32) | u64(time.dwLowDateTime)) * 100;
}


#ifndef Y_NO_PSAPI
Y_WIN_DYN_FUNC(psapi, GetProcessMemoryInfo, BOOL WINAPI (*func_ptr)(_In_ HANDLE,_Out_ PPROCESS_MEMORY_COUNTERS,_In_ DWORD))
#endif

#undef Y_WIN_DYN_FUNC

}
}


#endif // Y_OS_WIN
#endif // Y_UTILS_OS_WIN_H
