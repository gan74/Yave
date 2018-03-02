/*******************************
Copyright (c) 2016-2018 Grégoire Angerand

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

	public:
		static Duration seconds(double s);
		static Duration milliseconds(double ms);
		static Duration nanoseconds(u64 ns);

		explicit Duration(u64 seconds = 0, u32 subsec_nanos = 0);

		u64 to_nanos() const;
		double to_micros() const;
		double to_millis() const;
		double to_secs() const;

		u64 seconds() const;
		u32 subsec_nanos() const;

		bool operator<(const Duration& other) const;
		bool operator<=(const Duration& other) const;
		bool operator>(const Duration& other) const;
		bool operator>=(const Duration& other) const;

	private:
		u64 _secs;
		u32 _subsec_ns;
};


class Chrono {

	public:
		Chrono();

		void start();
		Duration reset();

		Duration elapsed() const;

		static Duration program();

	private:
		std::chrono::time_point<std::chrono::high_resolution_clock> _time;
};

class DebugTimer : NonCopyable {

	public:
		DebugTimer(const String& msg, const Duration& minimum = Duration());
		~DebugTimer();

	private:
		String _msg;
		Chrono _chrono;
		Duration _minimum;
};


}
}

#endif //Y_CORE_CHRONO_H
