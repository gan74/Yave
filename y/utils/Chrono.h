/*******************************
Copyright (C) 2013-2016 gregoire ANGERAND

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


#ifndef Y_UTILS_CHRONO_H
#define Y_UTILS_CHRONO_H

#include "types.h"
#include <chrono>

namespace y {

class Duration {
	public:
		Duration(u64 seconds, u32 subsec_nanos) : _secs(seconds), _subsec_ns(subsec_nanos) {
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


}

#endif //Y_CHRONO_H
