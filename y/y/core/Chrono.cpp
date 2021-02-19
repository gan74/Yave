/*******************************
Copyright (c) 2016-2021 Grégoire Angerand

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

#include "Chrono.h"

#include <y/utils/log.h>
#include <y/utils/format.h>

#include <thread>
#include <chrono>

#ifdef Y_OS_WIN
#include <windows.h>
#endif

namespace y {
namespace core {

u64 Duration::to_nanos() const {
    return _secs * 1000000000 + _subsec_ns;
}

double Duration::to_micros() const {
    return _secs * 1000000 + _subsec_ns / 1000.0;
}

double Duration::to_millis() const {
    return _secs * 1000 + _subsec_ns / 1000000.0;
}

double Duration::to_secs() const {
    return _secs + _subsec_ns / 1000000000.0;
}

u64 Duration::seconds() const {
    return _secs;
}

u32 Duration::subsec_nanos() const {
    return _subsec_ns;
}

bool Duration::operator<(const Duration& other) const {
    return std::tie(_secs, _subsec_ns) < std::tie(other._secs, other._subsec_ns);
}

bool Duration::operator<=(const Duration& other) const {
    return std::tie(_secs, _subsec_ns) <= std::tie(other._secs, other._subsec_ns);
}

bool Duration::operator>(const Duration& other) const {
    return std::tie(_secs, _subsec_ns) > std::tie(other._secs, other._subsec_ns);
}

bool Duration::operator>=(const Duration& other) const {
    return std::tie(_secs, _subsec_ns) >= std::tie(other._secs, other._subsec_ns);
}


void Duration::sleep(const Duration& dur) {
    const u64 micros = dur.subsec_nanos() / 1000 + dur.seconds() * 1000000;
    std::this_thread::sleep_for(std::chrono::microseconds(micros));
}


using Nano = std::chrono::nanoseconds;

Chrono::Chrono() {
#ifdef Y_OS_WIN
    LARGE_INTEGER li;
    QueryPerformanceFrequency(&li);
    _freq = li.QuadPart;
#endif
    start();
}

void Chrono::start() {
#ifdef Y_OS_WIN
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    _counter = li.QuadPart;
#else
    _time = std::chrono::high_resolution_clock::now();
#endif
}

Duration Chrono::reset() {
    auto e = elapsed();
    start();
    return e;
}

Duration Chrono::elapsed() const {
#ifdef Y_OS_WIN
    LARGE_INTEGER li;
    QueryPerformanceCounter(&li);
    const u64 diff = li.QuadPart -_counter;
    return Duration::seconds(diff / double(_freq));
    //return Duration(diff / _freq, u32((double(diff) / _freq) * 1000000000.0));
#else
    const auto nanos = u64(std::chrono::duration_cast<Nano>(std::chrono::high_resolution_clock::now() - _time).count());
    return Duration(nanos / 1000000000, nanos % 1000000000);
#endif
}

Duration Chrono::program() {
    static Chrono timer;
    return timer.elapsed();
}


DebugTimer::DebugTimer(const String& msg, const Duration& minimum) : _msg(msg), _minimum(minimum) {
}

DebugTimer::~DebugTimer() {
    if(const auto time = _chrono.elapsed(); time >= _minimum) {
        log_msg(fmt("%: % ms", _msg, time.to_millis()), Log::Perf);
    }
}

Duration DebugTimer::elapsed() const {
    return _chrono.elapsed();
}


}
}

