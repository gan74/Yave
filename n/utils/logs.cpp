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
#include <n/core/Map.h>
#include <n/core/Timer.h>
#include <n/core/String.h>
#include <n/concurrent/SpinLock.h>
#include <n/concurrent/LockGuard.h>
#include <n/io/TextOutputStream.h>
#include <n/concurrent/Thread.h>
#include <n/concurrent/Mutex.h>
#include <n/io/File.h>
#include <iostream>

namespace n {

const core::Timer globalTimer;

struct PerfEvent
{
	core::String name;
	double time;
	bool beg;
};

thread_local core::Array<PerfEvent> *threadEvents;
core::Map<void *, core::Array<PerfEvent>*> threads;
concurrent::Mutex eventMutex;

void pushPerfData(const core::String &name, bool b) {
	if(!threadEvents) {
		N_LOCK(eventMutex);
		threadEvents = new core::Array<PerfEvent>();
		threads[concurrent::Thread::getCurrent()] = threadEvents;
	}
	threadEvents->append(PerfEvent{name, globalTimer.elapsed(), b});
}

core::String simpler(const core::String &str) {
	return str/*.replaced("n::", "").replaced("core::", "").replaced("graphics::", "")*/;
}

template<typename T>
core::String toString(const core::String &name, T value) {
	return "\"" + name + "\":\"" + value + "\"";
}

void dumpThreadPerfData(uint pid, void *tid, const core::Array<PerfEvent> &events, io::TextOutputStream &stream) {
	for(const PerfEvent &e : events) {
		stream<<"{"<<toString("cat", "none")<<", "<<toString("pid", pid)<<", "<<toString("tid", tid)<<", "<<toString("ts", e.time * 1000000)<<", ";
		if(e.beg) {
			stream<<toString("ph", "B")<<", "<<toString("name", simpler(e.name));
		} else {
			stream<<toString("ph", "E");
		}
		stream<<"},\n";
	}
}

void dumpAllThreadPerfData(io::TextOutputStream &stream) {
	uint pid = SysInfo::pid();
	void *tid = concurrent::Thread::getCurrent();
	stream<<"{\"traceEvents\":[\n";
	for(auto p : threads) {
		dumpThreadPerfData(pid, p._1, *p._2, stream);
	}
	stream<<"{"<<toString("cat", "PerfLog")<<", "<<toString("pid", pid)<<", "<<toString("tid", tid)<<", "<<toString("ts", globalTimer.elapsed() * 1000000)<<", "<<toString("name", simpler(__FUNC__))<<"}";
	stream<<"], \"displayTimeUnit\":\"ns\"}";
}

void dumpAllThreadPerfData() {
	io::File dump("./dump.json");
	if(dump.open(io::IODevice::Append)) {
		N_LOCK(eventMutex);
		io::TextOutputStream txt(&dump);
		dumpAllThreadPerfData(txt);
		dump.close();
	} else {
		logMsg("Unable to write performance data dump", ErrorLog);
	}
}


FunctionTimer::FunctionTimer(FunctionTimer &&t) : logEnd(false) {
	std::swap(logEnd, t.logEnd);
}

FunctionTimer::~FunctionTimer() {
	if(logEnd) {
		pushPerfData("", false);
	}
}

FunctionTimer::FunctionTimer(const core::String &nm) : logEnd(true) {
	pushPerfData(nm, true);
}


void logMsg(const core::String &msg, LogType type) {
	static concurrent::SpinLock lock;
	const core::String logTypes[] = {"[info] ", "[error] ", "[perf] "};
	N_LOCK(lock);

	std::cout<<logTypes[type]<<msg<<std::endl;
	lock.unlock();
}

FunctionTimer logPerf(const core::String &funName) {
	return FunctionTimer(funName);
}

}
