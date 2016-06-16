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

#ifndef N_UTILS_PERF_H
#define N_UTILS_PERF_H

#include <n/types.h>
#include <n/defines.h>

namespace n {
namespace io {
class SynchronizedOutputStream;
class Device;
}

class FunctionTimer : NonCopyable
{
	public:
		FunctionTimer(FunctionTimer &&t);
		~FunctionTimer();

	private:
		friend FunctionTimer logFuncPerf(const char *);
		FunctionTimer(const char *name);

		bool logEnd;
};

class JsonPerfTracer : NonCopyable
{
	enum TraceType
	{
		FuncBegin,
		FuncEnd,
		Event,
	};

	struct Trace
	{
		double time;
		TraceType type;
		const char *data;
	};


	public:
		static double getTime();

		JsonPerfTracer(io::SynchronizedOutputStream *out);
		~JsonPerfTracer();

		void begin(const char *name);
		void end();

	private:
		Trace traceBuffer[512];
		uint traces;
		io::SynchronizedOutputStream *stream;
		uint tid;
		static constexpr uint Size = sizeof(traceBuffer) / sizeof(traceBuffer[0]);

		void emptyBuffer();
};

FunctionTimer logFuncPerf(const char *name);
void setTraceOutputStream(io::Device *out);
void dumpThreadPerfData();
}

#ifdef N_PERF_LOG_ENABLED
#define N_LOG_PERF_LINE_HELPER(LINE) auto _perf_at_ ## LINE = n::logFuncPerf( __PRETTY_FUNCTION__)
#define N_LOG_PERF_HELPER(LINE) N_LOG_PERF_LINE_HELPER(LINE)
#define N_LOG_PERF N_LOG_PERF_HELPER(__LINE__)
#else
#define N_LOG_PERF do {} while(0)
#endif


#endif // N_UTILS_PERF_H

