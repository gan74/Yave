/*******************************
Copyright (c) 2016-2025 Grégoire Angerand

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

#ifndef Y_OS_WIN
#include <chrono>
#endif

namespace y {
namespace core {

class Duration {
    static constexpr Duration div(double s, double div) {
        const double nano_div = 1'000'000'000 / div;
        const u64 secs = u64(s / div);
        s -= secs * div;
        return Duration(secs, u32(s * nano_div));
    }

    public:
        static constexpr Duration seconds(double s) {
            return div(s, 1.0);
        }

        static constexpr Duration milliseconds(double ms) {
            return div(ms, 1'000.0);
        }

        static constexpr Duration microseconds(double ms) {
            return div(ms, 1'000'000.0);
        }


        static constexpr Duration nanoseconds(u64 ns) {
            return Duration(ns / 1'000'000'000, ns % 1'000'000'000);
        }

        explicit Duration() = default;

        static void sleep(const Duration& dur);

        u64 to_nanos() const;
        double to_micros() const;
        double to_millis() const;
        double to_secs() const;

        u64 seconds() const;
        u32 subsec_nanos() const;

        Duration operator+(const Duration& other) const;
        Duration& operator+=(const Duration& other);

        auto operator<=>(const Duration&) const = default;

    private:
        constexpr explicit Duration(u64 seconds, u32 subsec_nanos) : _secs(seconds), _subsec_ns(subsec_nanos) {
            y_debug_assert(subsec_nanos < 1'000'000'000);
        }

        u64 _secs = 0;
        u32 _subsec_ns = 0;
};


class StopWatch {

    public:
        StopWatch();

        void start();
        Duration reset();

        Duration elapsed() const;

        static Duration program();

    private:
#ifdef Y_OS_WIN
        u64 _counter;
        u64 _freq = 1;
#else
        std::chrono::time_point<std::chrono::steady_clock> _time;
#endif
};

class DebugTimer : NonCopyable {

    public:
        DebugTimer(const String& msg, const Duration& minimum = Duration());
        ~DebugTimer();

        Duration elapsed() const;

    private:
        String _msg;
        StopWatch _chrono;
        Duration _minimum;
};


}
}

#endif //Y_CORE_CHRONO_H

