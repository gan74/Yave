/*******************************
Copyright (c) 2016-2017 Grégoire Angerand

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
#ifndef Y_CORE_CHRONO_H
#define Y_CORE_CHRONO_H

#include "String.h"
#include <chrono>

namespace y {
namespace core {

class Duration {
	static Duration div(double s, double div) {
		double nano_div = 1000000000 / div;
		u64 secs = s / div;
		s -= secs * div;
		return Duration(secs, u32(s * nano_div));
	}

	public:
		static Duration seconds(double s) {
			return div(s, 1.0);
		}

		static Duration milliseconds(double ms) {
			return div(ms, 1000.0);
		}

		explicit Duration(u64 seconds = 0, u32 subsec_nanos = 0) : _secs(seconds), _subsec_ns(subsec_nanos) {
		}

		u64 to_nanos() const {
			return _secs * 1000000000 + _subsec_ns;
		}

		double to_micros() const {
			return _secs * 1000000 + _subsec_ns / 1000.0;
		}

		double to_millis() const {
			return _secs * 1000 + _subsec_ns / 1000000.0;
		}

		double to_secs() const {
			return _secs + _subsec_ns / 1000000000.0;
		}

		u64 seconds() const {
			return _secs;
		}

		u32 subsec_nanos() const {
			return _subsec_ns;
		}

		bool operator<(const Duration& other) const {
			return std::tie(_secs, _subsec_ns) < std::tie(other._secs, other._subsec_ns);
		}

		bool operator<=(const Duration& other) const {
			return std::tie(_secs, _subsec_ns) <= std::tie(other._secs, other._subsec_ns);
		}

		bool operator>(const Duration& other) const {
			return std::tie(_secs, _subsec_ns) > std::tie(other._secs, other._subsec_ns);
		}

		bool operator>=(const Duration& other) const {
			return std::tie(_secs, _subsec_ns) >= std::tie(other._secs, other._subsec_ns);
		}

	private:
		u64 _secs;
		u32 _subsec_ns;
};


class Chrono {
	using Nano = std::chrono::nanoseconds;

	public:
		Chrono() {
			start();
		}

		void start() {
			_time = std::chrono::high_resolution_clock::now();
		}

		Duration reset() {
			auto e = elapsed();
			start();
			return e;
		}

		Duration elapsed() const {
			auto nanos = u64(std::chrono::duration_cast<Nano>(std::chrono::high_resolution_clock::now() - _time).count());
			return Duration(nanos / 1000000000, nanos % 1000000000);
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> _time;
};

class DebugTimer : NonCopyable {

	public:
		DebugTimer(const String& msg, const Duration& minimum = Duration()) : _msg(msg), _minimum(minimum) {
		}

		~DebugTimer() {
			auto time = _chrono.elapsed();
			if(time >= _minimum) {
				log_msg(_msg + ": " + time.to_millis() + "ms", LogType::Perf);
			}
		}

	private:
		String _msg;
		Chrono _chrono;
		Duration _minimum;
};


}
}

#endif //Y_CORE_CHRONO_H
