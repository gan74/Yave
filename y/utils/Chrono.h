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

namespace y {

class Duration {
	public:
		Duration(u64 seconds, u32 subsec_nanos) : secs(seconds), subsec_ns(subsec_nanos) {
		}

		u64 to_nanos() const {
			return secs * 1000000000 + subsec_ns;
		}

		double to_micros() const {
			return secs * 1000000 + subsec_ns / 1000.0;
		}

		double to_millis() const {
			return secs * 1000 + subsec_ns / 1000000.0;
		}

		double to_secs() const {
			return secs + subsec_ns / 1000000000.0;
		}

		u64 seconds() const {
			return secs;
		}

		u32 subsec_nanos() const {
			return subsec_ns;
		}

	private:
		u64 secs;
		u32 subsec_ns;
};


class Chrono {
	using Nano = std::chrono::nanoseconds;

	public:
		Chrono() {
			start();
		}

		void start() {
			time = std::chrono::high_resolution_clock::now();
		}

		Duration reset() {
			auto e = elapsed();
			start();
			return e;
		}

		Duration elapsed() const {
			auto nanos = std::chrono::duration_cast<Nano>(std::chrono::high_resolution_clock::now() - time).count();
			return Duration(nanos / 1000000000, nanos % 1000000000);
		}

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> time;
};


}

#endif //Y_CHRONO_H
