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
#include <iostream>

#ifdef Y_OS_WIN
#include <windows.h>

#ifndef Y_NO_PSAPI
#include <psapi.h>
#endif

namespace y {
namespace windows {

template<const char *name>
HMODULE library() {
	static HMODULE *h = 0;
	if(!h) {
		wchar_t buffer[256];
		swprintf(buffer, 256, L"%hs", name);
		auto lib = LoadLibrary(buffer);
		if(!lib) {
			fatal("Unable to load library");
		}
		h = new HMODULE(lib);
	}
	return *h;
}

static constexpr const char psapi[] = "psapi";

template<typename... Args>
bool call(const char *name, Args... args) {
	auto func_ptr = reinterpret_cast<void (*)(Args...)>(GetProcAddress(library<psapi>(), name));
	if(!func_ptr) {
		return false;
	}
	func_ptr(args...);
	return true;
}

}
}

#endif // Y_OS_WIN

#endif // Y_UTILS_OS_WIN_H
