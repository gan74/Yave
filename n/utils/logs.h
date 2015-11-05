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

#ifndef N_UTILS_LOG
#define N_UTILS_LOG

#include <n/types.h>
#include <n/defines.h>

namespace n {
namespace io {
class TextOutputStream;
}

enum LogType
{
	InfoLog,
	ErrorLog,
	PerfLog
};

class FunctionTimer : NonCopyable
{
	public:
		FunctionTimer(FunctionTimer &&t);
		~FunctionTimer();

	private:
		friend FunctionTimer logFuncPerf(const core::String &);
		FunctionTimer(const core::String &nm);

		bool logEnd;
};

void logMsg(const core::String &msg, LogType type = InfoLog);
FunctionTimer logFuncPerf(const core::String &funName);
void dumpAllThreadPerfData();
void dumpAllThreadPerfData(io::TextOutputStream &stream);
}

#ifdef N_PERF_LOG_ENABLED
#define N_LOG_PERF auto _perf = n::logFuncPerf( __PRETTY_FUNCTION__)
#else
#define N_LOG_PERF
#endif

#endif // N_UTILS_LOG

