/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.
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
