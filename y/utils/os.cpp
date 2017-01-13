/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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

#include "os.h"
#include "os_win.h"

#include <y/utils.h>

#include <iostream>

namespace y {
namespace os {

usize pid() {
	#ifdef Y_OS_WIN
	return GetCurrentProcessId();
	#else
	return getpid();
	#endif
}

MemInfo phys_mem_info() {
	#ifdef Y_OS_WIN
	MEMORYSTATUSEX info;
	info.dwLength = sizeof(info);
	GlobalMemoryStatusEx(&info);
	return MemInfo { info.ullTotalPhys, info.ullAvailPhys };
	#else
	return MemInfo { 0, 0 };
	#endif
}

usize mem_usage() {
	#if defined(Y_OS_WIN) && !defined(Y_NO_PSAPI)
	PROCESS_MEMORY_COUNTERS info;
	info.cb = sizeof(info);
	return windows::GetProcessMemoryInfo_func()(GetCurrentProcess(), &info, DWORD(sizeof(info))) ? info.WorkingSetSize : 0;
	#else
	return 0;
	#endif
}


u64 get_user_time_ns() {
	#ifdef Y_OS_WIN
	FILETIME time;
	FILETIME garbage;
	GetProcessTimes(GetCurrentProcess(), &garbage, &garbage, &garbage, &time);
	return windows::filetime_to_ns(time);
	#else
	return 0;
	#endif
}

u64 get_kernel_time_ns() {
	#ifdef Y_OS_WIN
	FILETIME time;
	FILETIME garbage;
	GetProcessTimes(GetCurrentProcess(), &garbage, &garbage, &time,&garbage);
	return windows::filetime_to_ns(time);
	#else
	return 0;
	#endif
}

AppTimes get_times_ns() {
	#ifdef Y_OS_WIN
	FILETIME user;
	FILETIME kernel;
	FILETIME garbage;
	GetProcessTimes(GetCurrentProcess(), &garbage, &garbage, &kernel, &user);
	return AppTimes { windows::filetime_to_ns(kernel), windows::filetime_to_ns(user) };
	#else
	return AppTimes { 0, 0 };
	#endif
}

}

}


#ifdef Y_OS_WIN

/*y_on_startup() {
	SetConsoleOutputCP(CP_UTF8);
}*/

#endif
