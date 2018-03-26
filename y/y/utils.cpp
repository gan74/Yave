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

#include "utils.h"
#include <y/core/String.h>

#ifdef __GNUG__
#include <cstdlib>
#include <memory>
#include <cxxabi.h>
#endif

namespace y {

namespace detail {

#ifdef __GNUG__
	core::String demangle_type_name(const char* name) {
		int status = 0;
		char* d = abi::__cxa_demangle(name, nullptr, nullptr, &status);
		if(status) {
			return core::String(name);
		}

		return core::String::from_owned(d);
	}
#else
	core::String demangle_type_name(const char* name) {
		return core::String(name);
	}
#endif
}


Nothing fatal(const char* msg, const char* file, int line) {
	core::String msg_str(msg);
	if(file) {
		msg_str += " in file \""_s + file + "\"";
	}
	if(line) {
		msg_str += " at line "_s + line;
	}
	log_msg(msg_str, Log::Error);
	std::abort();
}

}
