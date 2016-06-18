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


#include "utils.h"
#include <n/core/String.h>
#include <n/concurrent/Atomic.h>
#include <n/defines.h>

namespace n {

void fatal(const char *msg, const char *file, int line) {
	logMsg(msg + (file ? core::String(" in file ") + file + (line ? core::String(" at line ") + line : core::String()) : core::String()), ErrorLog);
	exit(1);
}

uint uniqueId() {
	static concurrent::auint counter;
	return ++counter;
}

}
