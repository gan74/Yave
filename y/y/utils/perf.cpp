/*******************************
Copyright (c) 2016-2020 Grégoire Angerand

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

#include <atomic>
#include <thread>
#include <mutex>
#include <cstdio>

#include <list>

namespace y {
namespace perf {


#ifdef Y_PERF_LOG_ENABLED

struct ThreadData;

static std::mutex mutex;
static std::unique_ptr<io2::File> output_file;

static constexpr usize buffer_size = 256 * 1024;
static constexpr usize print_buffer_len = 512;
static thread_local ThreadData* thread_local_ptr = nullptr;
static std::list<ThreadData> threads;
static std::atomic<bool> capturing = false;

namespace {
static struct Guard {
	~Guard() {
		if(is_capturing()) {
			y_fatal("Capture still in progress.");
		}
	}

	void use() {
	}
} guard;
}


static double micros() {
	return core::Chrono::program().to_micros();
}

static u32 thread_id() {
	static std::atomic<u32> id = 0;
	static thread_local u32 tid = ++id;
	return tid;
}

void start_capture(const char* out_filename) {
	auto file = std::make_unique<io2::File>(std::move(io2::File::create(out_filename).expected("Unable to open output file.")));

	{
		const std::string_view start = R"({"traceEvents":[)";
		file->write(start.data(), start.size()).expected("Unable to write perf dump.");
	}

	const std::unique_lock lock(mutex);

	if(is_capturing()) {
		y_fatal("Capture already in progress.");
	}

	output_file = std::move(file);
	guard.use();
	capturing = true;
}


void end_capture() {
	std::unique_lock lock(mutex);

	if(!is_capturing()) {
		y_fatal("Not capturing.");
	}

	capturing = false;
	threads.clear();

	{
		char b[print_buffer_len];
		const usize len = std::snprintf(b, sizeof(b), R"({"name":"capture ended","cat":"perf","ph":"i","pid":0,"tid":%u,"ts":%f}]})", thread_id(), micros());
		output_file->write(b, len).expected("Unable to write perf dump.");
	}

	output_file = nullptr;
}

bool is_capturing() {
	return capturing;
}

struct ThreadData final : NonMovable {
	std::unique_ptr<u8[]> buffer;
	usize buffer_offset = 0;

	ThreadData() : buffer(std::make_unique<u8[]>(buffer_size)) {
		y_debug_assert(thread_local_ptr == nullptr);
		thread_local_ptr = this;
	}

	~ThreadData() {
		y_debug_assert(thread_local_ptr == this);
		thread_local_ptr = nullptr;
		flush();
	}

	void write(const char* str, usize len) {
		const usize remaining = buffer_size - buffer_offset;
		if(len >= remaining) {
			const std::unique_lock lock(mutex);
			flush();
		}
		std::memcpy(buffer.get() + buffer_offset, str, len);
		buffer_offset += len;
	}

	void flush() {
		y_debug_assert(!mutex.try_lock());
		y_debug_assert(output_file);
		output_file->write(buffer.get(), buffer_offset).expected("Unable to write perf dump.");
		buffer_offset = 0;
	}
};

static ThreadData& thread_data() {
	if(!thread_local_ptr) {
		std::unique_lock lock(mutex);
		threads.emplace_back();
	}
	y_debug_assert(thread_local_ptr);
	return *thread_local_ptr;
}

static int paren(const char* buff) {
	if(const char* p = std::strchr(buff, '('); p) {
		return p - buff;
	}
	return print_buffer_len;
}

void enter(const char* cat, const char* func) {
	if(!is_capturing()) {
		return;
	}
	char b[print_buffer_len];
	const usize len = std::snprintf(b, sizeof(b), R"({"name":"%.*s","cat":"%s","ph":"B","pid":0,"tid":%u,"ts":%f},)", paren(func), func, cat, thread_id(), micros());
	if(len >= sizeof(b)) {
		y_fatal("Too long.");
	}
	thread_data().write(b, len);
}

void leave(const char* cat, const char* func) {
	if(!is_capturing()) {
		return;
	}
	char b[print_buffer_len];
	const usize len = std::snprintf(b, sizeof(b), R"({"name":"%.*s","cat":"%s","ph":"E","pid":0,"tid":%u,"ts":%f},)", paren(func), func, cat, thread_id(), micros());
	if(len >= sizeof(b)) {
		y_fatal("Too long.");
	}
	thread_data().write(b, len);
}

void event(const char* cat, const char* name) {
	if(!is_capturing()) {
		return;
	}
	char b[print_buffer_len];
	const usize len = std::snprintf(b, sizeof(b), R"({"name":"%s","cat":"%s","ph":"I","pid":0,"tid":%u,"ts":%f},)", name, cat, thread_id(), micros());
	if(len >= sizeof(b)) {
		y_fatal("Too long.");
	}
	thread_data().write(b, len);
}


#else
void start_capture(const char*) {}
void end_capture() {}
bool is_capturing() { return false; }
void enter(const char*, const char*) {}
void leave(const char*, const char*) {}
void event(const char*, const char*) {}
#endif

}
}
