/*******************************
Copyright (c) 2016-2019 Grégoire Angerand

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
#include "log.h"
#include <y/utils.h>

#include <mutex>

#include <iostream>

namespace y {

static detail::log_callback callback = nullptr;
static void* callback_user_data = nullptr;

static std::mutex lock;

void log_msg(std::string_view msg, Log type) {
	static constexpr std::array<const char*, 5> log_type_str = {{"info", "warning", "error", "debug", "perf"}};
	std::lock_guard _(lock);

	if(callback && callback(msg, type, callback_user_data)) {
		return;
	}

	(type == Log::Error || type == Log::Warning ? std::cerr : std::cout) << "[" << log_type_str[usize(type)] << "] " << msg << std::endl;
}

void set_log_callback(detail::log_callback func, void* user_data) {
	std::lock_guard _(lock);
	callback = func;
	callback_user_data = user_data;
}

}
