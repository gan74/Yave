/*******************************
Copyright (c) 2016-2018 Gr�goire Angerand

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

namespace y {
namespace core {

static Duration div(double s, double div) {
	double nano_div = 1000000000 / div;
	u64 secs = s / div;
	s -= secs * div;
	return Duration(secs, u32(s * nano_div));
}

Duration Duration::seconds(double s) {
	return div(s, 1.0);
}

Duration Duration::milliseconds(double ms) {
	return div(ms, 1000.0);
}

Duration Duration::nanoseconds(u64 ns) {
	return Duration(ns / 100000000, ns % 100000000);
}

Duration::Duration(u64 seconds, u32 subsec_nanos) : _secs(seconds), _subsec_ns(subsec_nanos) {
}

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


using Nano = std::chrono::nanoseconds;

Chrono::Chrono() {
	start();
}

void Chrono::start() {
	_time = std::chrono::high_resolution_clock::now();
}

Duration Chrono::reset() {
	auto e = elapsed();
	start();
	return e;
}

Duration Chrono::elapsed() const {
	auto nanos = u64(std::chrono::duration_cast<Nano>(std::chrono::high_resolution_clock::now() - _time).count());
	return Duration(nanos / 1000000000, nanos % 1000000000);
}

Duration Chrono::program() {
	static Chrono timer;
	return timer.elapsed();
}


DebugTimer::DebugTimer(const String& msg, const Duration& minimum) : _msg(msg), _minimum(minimum) {
}

DebugTimer::~DebugTimer() {
	if(auto time = _chrono.elapsed(); time >= _minimum) {
		log_msg(_msg + ": " + time.to_millis() + "ms", Log::Perf);
	}
}


}
}
