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
#ifndef Y_UTILS_PERF_H
#define Y_UTILS_PERF_H

#include <y/core/Ptr.h>
#include <y/io/Writer.h>

#include <y/defines.h>

namespace y {
namespace perf {

// For use with chrome://tracing

void set_output_ptr(core::Unique<io::Writer>&& out);

template<typename T>
void set_output(T&& t) {
	set_output_ptr(core::Unique<io::Writer>(new T(std::forward<T>(t))));
}

void enter(const char* cat, const char* func);
void leave(const char* cat, const char* func);
void event(const char* cat, const char* name);

inline auto log_func(const char* cat, const char* func) {
	class Logger : NonCopyable {
		const char* _cat;
		const char* _func;
		public:
			Logger(const char* cat, const char* func) : _cat(cat), _func(func) {
				enter(cat, func);
			}
			~Logger() {
				leave(_cat, _func);
			}
	};

	return Logger(cat, func);
}

#ifdef Y_PERF_LOG_ENABLED
#define Y_LOG_PERF_LINE_HELPER(cat, LINE) auto _perf_at_ ## LINE = y::perf::log_func(cat, __PRETTY_FUNCTION__)
#define Y_LOG_PERF_HELPER(cat, LINE) Y_LOG_PERF_LINE_HELPER(cat, LINE)
#define Y_LOG_PERF(cat) Y_LOG_PERF_HELPER(cat, __LINE__)
#else
#define Y_LOG_PERF(cat) do {} while(0)
#endif

}
}

#endif // Y_UTILS_PERF_H
