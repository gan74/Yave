/*******************************
Copyright (C) 2013-2015 gregoire ANGERAND

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

#include "logs.h"
#include <n/core/String.h>
#include <n/concurrent/SpinLock.h>
#include <n/concurrent/LockGuard.h>
#include <iostream>

namespace n {

void logMsg(const core::String &msg, LogType type) {
	logMsg(msg, type);
}


void logMsg(const char *msg, LogType type) {
	static core::String logTypes[] = {"[info] ", "[error] ", "[warning] ", "[perf] ", "[debug] ", "[depr] "};
	static concurrent::SpinLock lock;

	N_LOCK(lock);
	std::cout << logTypes[type] << msg << std::endl;
}



}
