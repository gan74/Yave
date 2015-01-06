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

#ifndef N_CORE_TIMER_H
#define N_CORE_TIMER_H

#include <n/defines.h>
#include <chrono>

namespace n {
namespace core {

class Timer
{
	typedef std::chrono::duration<double, std::nano> Nano;

	public:
		Timer() : time(std::chrono::high_resolution_clock::now()) {
		}

		void start() {
			time = std::chrono::high_resolution_clock::now();
		}

		double reset() {
			double t = elapsed();
			start();
			return t;
		}

		double elapsed() const {
			return std::chrono::duration_cast<Nano>(std::chrono::high_resolution_clock::now() - time).count() / 1000000000;
		}

	private:
		 std::chrono::time_point<std::chrono::high_resolution_clock> time;
};

} //core
} //n


#endif // N_CORE_TIMER_H
