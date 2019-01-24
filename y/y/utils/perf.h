/*******************************
Copyright (c) 2016-2019 Gr�goire Angerand

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

#include <y/utils.h>
#include <memory>

namespace y {
namespace perf {

// For use with chrome://tracing

void set_output_file(const char* out);

void enter(const char* cat, const char* func);
void leave(const char* cat, const char* func);
void event(const char* cat, const char* name);

inline auto log_func(const char* func, const char* cat = "") {
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
#define y_profile() auto y_create_name_with_prefix(prof) = y::perf::log_func(__PRETTY_FUNCTION__)
#define y_profile_zone(name) auto y_create_name_with_prefix(prof) = y::perf::log_func(name)
#else
#define y_profile_zone(cat) do {} while(false)
#define y_profile() do {} while(false)
#endif

}
}

#endif // Y_UTILS_PERF_H
