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

#include "perf.h"
#include <y/core/Chrono.h>
#include <y/io2/File.h>

#include <thread>
#include <mutex>
#include <cstdio>

#include <list>

#include "os.h"

namespace y {
namespace perf {

#ifdef Y_PERF_LOG_ENABLED
static constexpr usize buffer_size = 16 * 1024;
static constexpr usize print_buffer_len = 512;

static std::mutex mutex;
static bool initialized = false;
static std::shared_ptr<io2::File> output = std::make_shared<io2::File>();
#endif

void set_output_file(const char* out) {
	unused(out);
#ifdef Y_PERF_LOG_ENABLED
	std::unique_lock lock(mutex);
	initialized = false;
	*output = std::move(io2::File::create(out).expected("Unable to open output file."));
#endif
}

#ifdef Y_PERF_LOG_ENABLED

static double micros() {
	return core::Chrono::program().to_micros();
}

static u32 thread_id() {
	static u32 id = 0;
	static thread_local u32 tid = ++id;
	return tid;
}

Y_TODO(TLS destructors not called on windows)

static thread_local struct ThreadData : NonMovable {
	std::shared_ptr<io2::File> thread_output;
	std::unique_ptr<char[]> buffer;
	usize buffer_offset = 0;
	u32 tid = 0;

	ThreadData() :
			thread_output(output),
			buffer(std::make_unique<char[]>(buffer_size)),
			tid(thread_id()) {
	}

	~ThreadData() {
		char b[print_buffer_len];
		usize len = std::snprintf(b, sizeof(b), R"({"name":"thread closed","cat":"perf","ph":"i","pid":0,"tid":%u,"ts":%f}]})", tid, micros());
		if(len >= sizeof(b)) {
			y_fatal("Too long.");
		}
		write(b, len);
		write_buffer();
		std::unique_lock lock(mutex);
		if(thread_output->is_open()) {
			thread_output->flush().ignore();
		}
	}

	void write(const char* str, usize len) {
		usize remaining = buffer_size - buffer_offset;
		if(len >= remaining) {
			write_buffer();
		}
		std::memcpy(buffer.get() + buffer_offset, str, len);
		buffer_offset += len;
	}

	void write_buffer() {
		std::unique_lock lock(mutex);
		try {
			if(thread_output->is_open() && buffer) {
				if(!initialized) {
					initialized = true;
					const char beg[] = R"({"traceEvents":[)";
					thread_output->write(reinterpret_cast<const u8*>(beg), sizeof(beg) - 1).ignore();
				}
				thread_output->write(reinterpret_cast<const u8*>(buffer.get()), buffer_offset).ignore();
				buffer_offset = 0;
				event("perf", "done writing buffer");
			}
		} catch(...) {
		}
	}
} thread_data;

static int paren(const char* buff) {
	if(const char* p = std::strchr(buff, '('); p) {
		return p - buff;
	}
	return print_buffer_len;
}

void enter(const char* cat, const char* func) {
	char b[print_buffer_len];
	usize len = std::snprintf(b, sizeof(b), R"({"name":"%.*s","cat":"%s","ph":"B","pid":0,"tid":%u,"ts":%f},)", paren(func), func, cat, thread_data.tid, micros());
	if(len >= sizeof(b)) {
		y_fatal("Too long.");
	}
	thread_data.write(b, len);
}

void leave(const char* cat, const char* func) {
	char b[print_buffer_len];
	usize len = std::snprintf(b, sizeof(b), R"({"name":"%.*s","cat":"%s","ph":"E","pid":0,"tid":%u,"ts":%f},)", paren(func), func, cat, thread_data.tid, micros());
	if(len >= sizeof(b)) {
		y_fatal("Too long.");
	}
	thread_data.write(b, len);
}

void event(const char* cat, const char* name) {
	char b[print_buffer_len];
	usize len = std::snprintf(b, sizeof(b), R"({"name":"%s","cat":"%s","ph":"i","pid":0,"tid":%u,"ts":%f},)", name, cat, thread_data.tid, micros());
	if(len >= sizeof(b)) {
		y_fatal("Too long.");
	}
	thread_data.write(b, len);
}

#endif

}
}
