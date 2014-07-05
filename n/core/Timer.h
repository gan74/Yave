/*******************************
Copyright (C) 2013-2014 grégoire ANGERAND

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

#ifndef N_USE_STD_TIME
	#if defined WIN32 || defined _WIN32 || defined __CYGWIN__
		#define N_USE_WIN_TIME
	#endif
#endif

#ifdef N_USE_WIN_TIME
#include <windows.h>

namespace n {
namespace core {

class Timer
{
	public:
		Timer() {
			start();
		}

		void start() {
			QueryPerformanceCounter(&time) ;
		}

		double startTime() const {
			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);
			return (double)time.QuadPart / (double)freq.QuadPart;
		}

		double reset() {
			double t = elapsed();
			start();
			return t;
		}

		double elapsed() const {
			LARGE_INTEGER t;
			QueryPerformanceCounter(&t) ;
			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);
			return (double)((t.QuadPart - time.QuadPart) / (double)freq.QuadPart);
		}

		static double step() {
			LARGE_INTEGER freq;
			QueryPerformanceFrequency(&freq);
			return 1.0 / (double)freq.QuadPart;
		}

	private:
		 LARGE_INTEGER time;
};

#else

#include <chrono>

class Timer
{
	public:
		Timer() : time(std::chrono::high_resolution_clock::now()) {
		}

		void start() {
			time = std::chrono::high_resolution_clock::now();
		}

		double startTime() const {
			return std::chrono::duration_cast<std::chrono::seconds>(time.time_since_epoch()).count();
		}

		double reset() {
			double t = elapsed();
			start();
			return t;
		}

		double elapsed() const {
			return (double)(std::chrono::duration_cast<std::chrono::seconds>(std::chrono::high_resolution_clock::now() - time)).count() / 1000;
		}

		static double step() {
			return (double)std::chrono::high_resolution_clock::period::num / std::chrono::high_resolution_clock::period::den;
		}


	private:
		 std::chrono::time_point<std::chrono::high_resolution_clock> time;
};

#endif

} //core
} //n
#endif // N_CORE_TIMER_H
