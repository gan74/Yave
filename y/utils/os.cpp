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

#include "os.h"
#include <y/utils.h>
#include <iostream>

#ifdef Y_OS_WIN
#include <windows.h>

#ifdef Y_USE_PSAPI
#include <psapi.h>
#endif
#endif

namespace y {

namespace os {

usize pid() {
	return getpid();
}

usize core_count() {
	#ifdef Y_OS_WIN
		SYSTEM_INFO info;
		GetSystemInfo(&info);
		return info.dwNumberOfProcessors;
	#else
		return 0;
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
	#if defined(Y_OS_WIN) && defined(Y_USE_PSAPI)
	PROCESS_MEMORY_COUNTERS info;
	info.cb = sizeof(info);
	GetProcessMemoryInfo(GetCurrentProcess(), &info, sizeof(info));
	return info.WorkingSetSize;
	#else
	return 0;
	#endif
}

}

}


#ifdef Y_OS_WIN

y_on_startup() {
	SetConsoleOutputCP(CP_UTF8);
}

#endif
