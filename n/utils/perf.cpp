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

#include "perf.h"
#include <n/core/Map.h>
#include <n/core/Timer.h>
#include <n/core/String.h>
#include <n/io/TextOutputStream.h>
#include <n/io/SynchronizedOutputStream.h>
#include <n/concurrent/Thread.h>
#include <n/concurrent/Mutex.h>
#include <n/concurrent/SpinLock.h>
#include <n/io/File.h>
#include <n/concurrent/LockGuard.h>

namespace n {

io::SynchronizedOutputStream *traceOut = 0;
thread_local JsonPerfTracer *tracer = 0;

struct TraceDumper
{
	~TraceDumper() {
		N_LOCK(lock);
		for(JsonPerfTracer *t : tracers) {
			delete t;
		}
		if(traceOut) {
			io::TextOutputStream stream(traceOut);
			stream << "{}], \"displayTimeUnit\":\"ms\"}";
			traceOut->flush();
		}
	}

	void addTracer(JsonPerfTracer *t) {
		N_LOCK(lock);
		tracers.append(t);
	}

	void removeTracer(JsonPerfTracer *t) {
		delete t;
		N_LOCK(lock);
		tracers.remove(t);
	}

	concurrent::SpinLock lock;
	core::Array<JsonPerfTracer *> tracers;
};

#ifdef N_PERF_LOG_ENABLED
TraceDumper dumper;
#endif

JsonPerfTracer *getThreadPerfTracer() {
	#ifdef N_PERF_LOG_ENABLED
	if(!tracer && traceOut) {
		tracer = new JsonPerfTracer(traceOut);
		dumper.addTracer(tracer);
	}
	return tracer;
	#else
	return 0;
	#endif
}

template<typename T>
static core::String tag(const core::String &name, T value) {
	return "\"" + name + "\":\"" + value + "\"";
}

double JsonPerfTracer::getTime() {
	static core::Timer global;
	return global.elapsed();
}

JsonPerfTracer::JsonPerfTracer(io::SynchronizedOutputStream *out) : traces(0), stream(out), tid(uint(concurrent::Thread::getCurrentId())) {
}

JsonPerfTracer::~JsonPerfTracer() {
	emptyBuffer();
}

void JsonPerfTracer::begin(const char *name) {
	traceBuffer[traces++] = Trace{getTime(), FuncBegin, name};
	if(traces == Size) {
		emptyBuffer();
	}
}

void JsonPerfTracer::end() {
	traceBuffer[traces++] = Trace{getTime(), FuncEnd, 0};
	if(traces == Size) {
		emptyBuffer();
	}
}

void JsonPerfTracer::emptyBuffer() {
	if(!stream) {
		traces = 0;
		return;
	}
	io::TextOutputStream out(stream);
	uint pid = SysInfo::pid();
	for(uint i = 0; i != traces; i++) {
		Trace e = traceBuffer[i];
		core::Array<core::String> trace;
		trace << "{";
		trace << tag("cat", "none") << ", ";
		trace << tag("pid", pid) << ", ";
		trace << tag("tid", tid) << ", ";
		trace << tag("ts", e.time * 1000000) << ", "; // ?????
		switch(e.type) {
			case FuncBegin:
				trace << tag("ph", "B") << ", ";
				trace << tag("name", e.data);
				break;

			case FuncEnd:
				trace << tag("ph", "E");
				break;

			case Event:
				trace << tag("ph", "I");
				trace << tag("name", e.data);
				break;
		}
		trace << "},\n";
		for(auto x : trace) {
			out << x;
		}
	}
	traces = 0;
}

FunctionTimer::FunctionTimer(FunctionTimer &&t) : logEnd(false) {
	std::swap(logEnd, t.logEnd);
}

FunctionTimer::~FunctionTimer() {
	if(logEnd) {
		auto *t = getThreadPerfTracer();
		if(t) {
			t->end();
		}
	}
}

FunctionTimer::FunctionTimer(const char *name) {
	auto *t = getThreadPerfTracer();
	logEnd = t;
	if(t) {
		t->begin(name);
	}
}

FunctionTimer logFuncPerf(const char *name) {
	return FunctionTimer(name);
}

io::Device *openDevice(io::Device *d) {
	if(d) {
		if(!(d->getOpenMode() & io::Device::Append)) {
			d->open(io::Device::Append);
		}
		if(!d->isOpen()) {
			d = 0;
		}
	}
	return d;
}

void setTraceOutputStream(io::Device *out) {
	out = openDevice(out);
	#ifdef N_PERF_LOG_ENABLED
	if(!traceOut && out) {
		io::SynchronizedOutputStream *t = new io::SynchronizedOutputStream(out);
		io::TextOutputStream stream(t);
		stream << "{\"traceEvents\":[\n";
		traceOut = t;
	}
	#else
	if(out) {
		io::TextOutputStream stream(out);
		stream << "Trace output stream specified while N_PERF_LOG_ENABLED is not defined.";
		out->close();
	}
	logMsg("Trace output stream specified while N_PERF_LOG_ENABLED is not defined.", WarningLog);
	#endif
}

void dumpThreadPerfData() {
	#ifdef N_PERF_LOG_ENABLED
	if(tracer) {
		dumper.removeTracer(tracer);
		tracer = 0;
	}
	#endif
}


}
