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

#ifndef N_UTILS_LOGS_H
#define N_UTILS_LOGS_H

#include <n/types.h>
#include <n/defines.h>

namespace n {
enum LogType
{
	InfoLog,
	ErrorLog,
	WarningLog,
	PerfLog,
	DebugLog,
	DeprecatedLog
};

void logMsg(const core::String &msg, LogType type = InfoLog);
void logMsg(const char *msg, LogType type = InfoLog);
}

#define N_DEPRECATED do {static bool _warn = false; if(!_warn) { _warn = true; n::logMsg(__PRETTY_FUNCTION__ + n::core::String(" is deprecated."), n::DeprecatedLog); }} while(0)

#endif // N_UTILS_LOGS_H

